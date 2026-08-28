#include "sovereign/milp/milp_model.hpp"

namespace sovereign::milp {

MILPModel::MILPModel(const lp::LPModel& lp_model) : lp_model_(lp_model) {
    Index n = lp_model_.num_variables();
    var_types_.resize(n, VariableType::Continuous);
    for (Index j = 0; j < n; ++j) {
        var_types_[j] = lp_model_.get_variable(j).type;
    }
}

MILPModel::MILPModel(std::string name, ObjectiveSense sense) : lp_model_(std::move(name), sense) {}

Index MILPModel::add_variable(
    const std::string& name,
    Float64 lb,
    Float64 ub,
    Float64 obj_coeff,
    VariableType type
) {
    Index idx = lp_model_.add_variable(name, lb, ub, obj_coeff, type);
    if (idx >= static_cast<Index>(var_types_.size())) {
        var_types_.resize(idx + 1, VariableType::Continuous);
    }
    var_types_[idx] = type;
    return idx;
}

Index MILPModel::add_constraint(
    const std::string& name,
    lp::ConstraintSense sense,
    Float64 rhs,
    std::span<const Index> col_indices,
    std::span<const Float64> values
) {
    return lp_model_.add_constraint(name, sense, rhs, col_indices, values);
}

void MILPModel::set_variable_type(Index var_idx, VariableType type) {
    if (var_idx >= 0 && var_idx < num_variables()) {
        var_types_[var_idx] = type;
        lp_model_.get_variable(var_idx).type = type;
    }
}

VariableType MILPModel::variable_type(Index var_idx) const {
    if (var_idx >= 0 && var_idx < static_cast<Index>(var_types_.size())) {
        return var_types_[var_idx];
    }
    return VariableType::Continuous;
}

bool MILPModel::is_integer(Index var_idx) const {
    VariableType t = variable_type(var_idx);
    return t == VariableType::Integer || t == VariableType::Binary;
}

bool MILPModel::is_binary(Index var_idx) const {
    return variable_type(var_idx) == VariableType::Binary;
}

bool MILPModel::is_continuous(Index var_idx) const {
    return variable_type(var_idx) == VariableType::Continuous;
}

std::vector<Index> MILPModel::integer_variable_indices() const {
    std::vector<Index> res;
    Index n = num_variables();
    for (Index j = 0; j < n; ++j) {
        if (is_integer(j)) {
            res.push_back(j);
        }
    }
    return res;
}

Index MILPModel::num_integer_variables() const {
    Index count = 0;
    Index n = num_variables();
    for (Index j = 0; j < n; ++j) {
        if (is_integer(j)) count++;
    }
    return count;
}

} // namespace sovereign::milp
