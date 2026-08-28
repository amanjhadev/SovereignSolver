#ifndef SOVEREIGN_LP_LP_MODEL_HPP
#define SOVEREIGN_LP_LP_MODEL_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/csr_matrix.hpp"
#include <span>
#include <string>
#include <vector>

namespace sovereign::lp {

enum class ConstraintSense : std::uint8_t {
    LessEqual    = 0, // <=
    GreaterEqual = 1, // >=
    Equal        = 2, // ==
    Range        = 3  // lhs <= row <= rhs
};

enum class VariableRole : std::uint8_t {
    Structural = 0,
    Slack      = 1,
    Surplus    = 2,
    Range      = 3,
    FreePlus   = 4,
    FreeMinus  = 5
};

struct Variable {
    std::string name;
    Float64 lower_bound{-INFINITY_VAL};
    Float64 upper_bound{INFINITY_VAL};
    Float64 obj_coeff{0.0};
    VariableType type{VariableType::Continuous};
    VariableRole role{VariableRole::Structural};
    Index original_var_index{-1};
    Index constraint_index{-1};
};

struct Constraint {
    std::string name;
    ConstraintSense sense{ConstraintSense::LessEqual};
    Float64 lhs{-INFINITY_VAL}; // Lower bound for row
    Float64 rhs{0.0};           // Upper bound for row
};

struct VariableMapEntry {
    Index original_index{-1};
    Index std_pos_index{-1};
    Index std_neg_index{-1}; // -1 if not free
    bool is_free{false};
};

class StandardFormMap {
private:
    std::vector<VariableMapEntry> var_map_;

public:
    StandardFormMap() = default;
    void add_entry(Index orig_idx, Index std_pos_idx, Index std_neg_idx = -1, bool is_free = false) {
        var_map_.push_back({orig_idx, std_pos_idx, std_neg_idx, is_free});
    }
    const std::vector<VariableMapEntry>& entries() const noexcept { return var_map_; }

    std::vector<Float64> recover_original_solution(std::span<const Float64> std_x) const {
        std::vector<Float64> orig_x(var_map_.size(), 0.0);
        for (std::size_t i = 0; i < var_map_.size(); ++i) {
            const auto& entry = var_map_[i];
            if (entry.is_free) {
                Float64 pos_val = (entry.std_pos_index >= 0 && entry.std_pos_index < static_cast<Index>(std_x.size())) ? std_x[entry.std_pos_index] : 0.0;
                Float64 neg_val = (entry.std_neg_index >= 0 && entry.std_neg_index < static_cast<Index>(std_x.size())) ? std_x[entry.std_neg_index] : 0.0;
                orig_x[i] = pos_val - neg_val;
            } else {
                Float64 val = (entry.std_pos_index >= 0 && entry.std_pos_index < static_cast<Index>(std_x.size())) ? std_x[entry.std_pos_index] : 0.0;
                orig_x[i] = val;
            }
        }
        return orig_x;
    }
};

class LPModel {
private:
    std::string name_{"unnamed_model"};
    ObjectiveSense obj_sense_{ObjectiveSense::Minimize};
    Float64 obj_offset_{0.0};

    std::vector<Variable> variables_;
    std::vector<Constraint> constraints_;

    StandardFormMap std_map_;

    // Constraint Matrix stored in COO during building, converted to CSC/CSR
    mutable linalg::COOMatrix matrix_coo_;
    mutable bool csc_dirty_{true};
    mutable linalg::CSCMatrix matrix_csc_;
    mutable linalg::CSRMatrix matrix_csr_;

public:
    LPModel() = default;
    explicit LPModel(std::string name, ObjectiveSense sense = ObjectiveSense::Minimize);

    // Model Building API
    void set_name(std::string name) { name_ = std::move(name); }
    void set_objective_sense(ObjectiveSense sense) { obj_sense_ = sense; }
    void set_objective_offset(Float64 offset) { obj_offset_ = offset; }

    Index add_variable(std::string name, Float64 lb = 0.0, Float64 ub = INFINITY_VAL, Float64 obj_coeff = 0.0, VariableType type = VariableType::Continuous);
    Index add_constraint(std::string name, ConstraintSense sense, Float64 rhs, std::span<const Index> col_indices, std::span<const Float64> values);
    Index add_constraint_range(std::string name, Float64 lhs, Float64 rhs, std::span<const Index> col_indices, std::span<const Float64> values);

    void add_matrix_entry(Index row, Index col, Float64 value);

    // Inspection API
    const std::string& name() const noexcept { return name_; }
    ObjectiveSense objective_sense() const noexcept { return obj_sense_; }
    Float64 objective_offset() const noexcept { return obj_offset_; }

    Index num_variables() const noexcept { return static_cast<Index>(variables_.size()); }
    Index num_constraints() const noexcept { return static_cast<Index>(constraints_.size()); }

    const Variable& get_variable(Index col) const;
    Variable& get_variable(Index col);
    const Constraint& get_constraint(Index row) const;
    Constraint& get_constraint(Index row);

    const linalg::CSCMatrix& get_csc_matrix() const;
    const linalg::CSRMatrix& get_csr_matrix() const;

    const StandardFormMap& standard_form_map() const noexcept { return std_map_; }
    void set_standard_form_map(StandardFormMap map) { std_map_ = std::move(map); }

    // Standard Form Conversion
    // Converts general LP (with inequalities and bounds) into equality form:
    // A_std * x_std = b_std,  l_std <= x_std <= u_std
    LPModel convert_to_standard_form() const;
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_LP_MODEL_HPP
