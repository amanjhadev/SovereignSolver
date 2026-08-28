#include "sovereign/lp/lp_model.hpp"
#include <stdexcept>
#include <algorithm>

namespace sovereign::lp {

LPModel::LPModel(std::string name, ObjectiveSense sense)
    : name_(std::move(name)), obj_sense_(sense) {}

Index LPModel::add_variable(std::string name, Float64 lb, Float64 ub, Float64 obj_coeff, VariableType type) {
    Index col_idx = static_cast<Index>(variables_.size());
    variables_.push_back(Variable{std::move(name), lb, ub, obj_coeff, type});
    matrix_coo_.resize_dimensions(num_constraints(), num_variables());
    csc_dirty_ = true;
    return col_idx;
}

Index LPModel::add_constraint(std::string name, ConstraintSense sense, Float64 rhs, std::span<const Index> col_indices, std::span<const Float64> values) {
    if (col_indices.size() != values.size()) {
        throw std::invalid_argument("Column indices and values dimension mismatch in add_constraint");
    }

    Index row_idx = static_cast<Index>(constraints_.size());
    Float64 lhs = -INFINITY_VAL;
    if (sense == ConstraintSense::Equal) {
        lhs = rhs;
    } else if (sense == ConstraintSense::GreaterEqual) {
        lhs = rhs;
        rhs = INFINITY_VAL;
    }

    constraints_.push_back(Constraint{std::move(name), sense, lhs, rhs});
    matrix_coo_.resize_dimensions(num_constraints(), num_variables());

    matrix_coo_.reserve(matrix_coo_.num_nonzeros() + col_indices.size());
    for (std::size_t k = 0; k < col_indices.size(); ++k) {
        matrix_coo_.add_entry(row_idx, col_indices[k], values[k]);
    }

    csc_dirty_ = true;
    return row_idx;
}

Index LPModel::add_constraint_range(std::string name, Float64 lhs, Float64 rhs, std::span<const Index> col_indices, std::span<const Float64> values) {
    if (col_indices.size() != values.size()) {
        throw std::invalid_argument("Column indices and values dimension mismatch in add_constraint_range");
    }

    Index row_idx = static_cast<Index>(constraints_.size());
    constraints_.push_back(Constraint{std::move(name), ConstraintSense::Range, lhs, rhs});
    matrix_coo_.resize_dimensions(num_constraints(), num_variables());

    matrix_coo_.reserve(matrix_coo_.num_nonzeros() + col_indices.size());
    for (std::size_t k = 0; k < col_indices.size(); ++k) {
        matrix_coo_.add_entry(row_idx, col_indices[k], values[k]);
    }

    csc_dirty_ = true;
    return row_idx;
}

void LPModel::add_matrix_entry(Index row, Index col, Float64 value) {
    if (row < 0 || row >= num_constraints() || col < 0 || col >= num_variables()) {
        throw std::out_of_range("Matrix entry index out of range");
    }
    matrix_coo_.add_entry(row, col, value);
    csc_dirty_ = true;
}

const Variable& LPModel::get_variable(Index col) const {
    if (col < 0 || col >= static_cast<Index>(variables_.size())) {
        throw std::out_of_range("Variable index out of range");
    }
    return variables_[col];
}

Variable& LPModel::get_variable(Index col) {
    if (col < 0 || col >= static_cast<Index>(variables_.size())) {
        throw std::out_of_range("Variable index out of range");
    }
    return variables_[col];
}

const Constraint& LPModel::get_constraint(Index row) const {
    if (row < 0 || row >= static_cast<Index>(constraints_.size())) {
        throw std::out_of_range("Constraint index out of range");
    }
    return constraints_[row];
}

Constraint& LPModel::get_constraint(Index row) {
    if (row < 0 || row >= static_cast<Index>(constraints_.size())) {
        throw std::out_of_range("Constraint index out of range");
    }
    return constraints_[row];
}

const linalg::CSCMatrix& LPModel::get_csc_matrix() const {
    if (csc_dirty_) {
        matrix_csc_ = matrix_coo_.to_csc();
        matrix_csr_ = matrix_csc_.to_csr();
        csc_dirty_ = false;
    }
    return matrix_csc_;
}

const linalg::CSRMatrix& LPModel::get_csr_matrix() const {
    get_csc_matrix(); // Refresh if dirty
    return matrix_csr_;
}

LPModel LPModel::convert_to_standard_form() const {
    // Standard form is always canonical minimization: min c_std^T x_std
    LPModel std_model(name_ + "_std", ObjectiveSense::Minimize);
    Float64 sense_factor = (obj_sense_ == ObjectiveSense::Maximize) ? -1.0 : 1.0;
    std_model.set_objective_offset(sense_factor * obj_offset_);

    StandardFormMap std_map;

    struct OrigVarMapInfo {
        bool is_free{false};
        Index pos_col{-1};
        Index neg_col{-1};
    };
    std::vector<OrigVarMapInfo> orig_var_info(num_variables());

    // 1. Process variables: split free variables x = x_plus - x_minus
    for (Index j = 0; j < num_variables(); ++j) {
        const auto& var = variables_[j];
        bool is_free = (var.lower_bound <= -INFINITY_VAL / 2.0 && var.upper_bound >= INFINITY_VAL / 2.0);
        Float64 std_obj = sense_factor * var.obj_coeff;

        if (is_free) {
            Index pos_col = std_model.add_variable(var.name + "_pos", 0.0, INFINITY_VAL, std_obj, var.type);
            auto& pos_v = std_model.get_variable(pos_col);
            pos_v.role = VariableRole::FreePlus;
            pos_v.original_var_index = j;

            Index neg_col = std_model.add_variable(var.name + "_neg", 0.0, INFINITY_VAL, -std_obj, var.type);
            auto& neg_v = std_model.get_variable(neg_col);
            neg_v.role = VariableRole::FreeMinus;
            neg_v.original_var_index = j;

            orig_var_info[j] = {true, pos_col, neg_col};
            std_map.add_entry(j, pos_col, neg_col, true);
        } else {
            Index std_col = std_model.add_variable(var.name, var.lower_bound, var.upper_bound, std_obj, var.type);
            auto& std_v = std_model.get_variable(std_col);
            std_v.role = VariableRole::Structural;
            std_v.original_var_index = j;

            orig_var_info[j] = {false, std_col, -1};
            std_map.add_entry(j, std_col, -1, false);
        }
    }

    const auto& orig_csc = get_csc_matrix();

    // 2. Add slacks/surpluses for constraints
    for (Index i = 0; i < num_constraints(); ++i) {
        const auto& c = constraints_[i];
        std::vector<Index> row_cols;
        std::vector<Float64> row_vals;

        for (Index j = 0; j < num_variables(); ++j) {
            auto r_indices = orig_csc.get_col_row_indices(j);
            auto c_values  = orig_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_indices.size(); ++k) {
                if (r_indices[k] == i) {
                    Float64 val = c_values[k];
                    const auto& info = orig_var_info[j];
                    if (info.is_free) {
                        row_cols.push_back(info.pos_col);
                        row_vals.push_back(val);
                        row_cols.push_back(info.neg_col);
                        row_vals.push_back(-val);
                    } else {
                        row_cols.push_back(info.pos_col);
                        row_vals.push_back(val);
                    }
                }
            }
        }

        if (c.sense == ConstraintSense::LessEqual) {
            // A_i * x + s_i = rhs,  s_i >= 0
            Index slack_col = std_model.add_variable("slack_" + c.name, 0.0, INFINITY_VAL, 0.0);
            auto& slack_v = std_model.get_variable(slack_col);
            slack_v.role = VariableRole::Slack;
            slack_v.constraint_index = i;

            row_cols.push_back(slack_col);
            row_vals.push_back(1.0);
            std_model.add_constraint(c.name, ConstraintSense::Equal, c.rhs, row_cols, row_vals);
        } else if (c.sense == ConstraintSense::GreaterEqual) {
            // A_i * x - s_i = lhs,  s_i >= 0
            Index surplus_col = std_model.add_variable("surplus_" + c.name, 0.0, INFINITY_VAL, 0.0);
            auto& surplus_v = std_model.get_variable(surplus_col);
            surplus_v.role = VariableRole::Surplus;
            surplus_v.constraint_index = i;

            row_cols.push_back(surplus_col);
            row_vals.push_back(-1.0);
            std_model.add_constraint(c.name, ConstraintSense::Equal, c.lhs, row_cols, row_vals);
        } else if (c.sense == ConstraintSense::Range) {
            // A_i * x + s_i = rhs,  0 <= s_i <= (rhs - lhs)
            Float64 slack_ub = c.rhs - c.lhs;
            Index range_col = std_model.add_variable("range_" + c.name, 0.0, slack_ub, 0.0);
            auto& range_v = std_model.get_variable(range_col);
            range_v.role = VariableRole::Range;
            range_v.constraint_index = i;

            row_cols.push_back(range_col);
            row_vals.push_back(1.0);
            std_model.add_constraint(c.name, ConstraintSense::Equal, c.rhs, row_cols, row_vals);
        } else {
            // Equality: A_i * x = rhs
            std_model.add_constraint(c.name, ConstraintSense::Equal, c.rhs, row_cols, row_vals);
        }
    }

    std_model.set_standard_form_map(std_map);
    return std_model;
}

} // namespace sovereign::lp
