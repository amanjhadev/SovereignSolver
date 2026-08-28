#include "sovereign/milp/branching.hpp"
#include <cmath>
#include <algorithm>
#include <string>

namespace sovereign::milp {

bool is_integral(Float64 val, Float64 tol_integrality) noexcept {
    Float64 rounded = std::round(val);
    return std::abs(val - rounded) <= tol_integrality;
}

bool check_solution_integrality(
    std::span<const Float64> sol,
    std::span<const Index> integer_var_indices,
    Float64 tol_integrality
) {
    for (Index orig_idx : integer_var_indices) {
        if (orig_idx >= 0 && orig_idx < static_cast<Index>(sol.size())) {
            if (!is_integral(sol[orig_idx], tol_integrality)) {
                return false;
            }
        }
    }
    return true;
}

Index select_branching_variable(
    std::span<const Float64> sol,
    std::span<const Index> integer_var_indices,
    BranchingRule rule,
    Float64 tol_integrality
) {
    Index chosen_var = -1;

    if (rule == BranchingRule::FirstFractional) {
        for (Index orig_idx : integer_var_indices) {
            if (orig_idx >= 0 && orig_idx < static_cast<Index>(sol.size())) {
                if (!is_integral(sol[orig_idx], tol_integrality)) {
                    return orig_idx;
                }
            }
        }
    } else { // MostFractional
        Float64 max_frac = -1.0;
        for (Index orig_idx : integer_var_indices) {
            if (orig_idx >= 0 && orig_idx < static_cast<Index>(sol.size())) {
                Float64 val = sol[orig_idx];
                if (!is_integral(val, tol_integrality)) {
                    Float64 f = val - std::floor(val);
                    Float64 frac_dist = std::min(f, 1.0 - f);
                    if (frac_dist > max_frac) {
                        max_frac = frac_dist;
                        chosen_var = orig_idx;
                    }
                }
            }
        }
    }

    return chosen_var;
}

void BranchTranslationLayer::create_child_branch_constraints(
    const MILPModel& model,
    Index orig_idx,
    Float64 val,
    BBNode& left_child,
    BBNode& right_child
) const {
    Float64 floor_val = std::floor(val);
    Float64 ceil_val = std::ceil(val);

    VariableType var_type = model.variable_type(orig_idx);

    if (var_type == VariableType::Binary) {
        // Binary variable x_k in {0, 1}
        VariableBoundChange left_b;
        left_b.col_index = orig_idx;
        left_b.new_lower_bound = -INFINITY_VAL;
        left_b.new_upper_bound = 0.0;
        left_child.bound_changes.push_back(left_b);

        VariableBoundChange right_b;
        right_b.col_index = orig_idx;
        right_b.new_lower_bound = 1.0;
        right_b.new_upper_bound = INFINITY_VAL;
        right_child.bound_changes.push_back(right_b);
    } else {
        // General integer variable (bounded, shifted, or free)
        VariableBoundChange left_b;
        left_b.col_index = orig_idx;
        left_b.new_lower_bound = -INFINITY_VAL;
        left_b.new_upper_bound = floor_val;
        left_child.bound_changes.push_back(left_b);

        VariableBoundChange right_b;
        right_b.col_index = orig_idx;
        right_b.new_lower_bound = ceil_val;
        right_b.new_upper_bound = INFINITY_VAL;
        right_child.bound_changes.push_back(right_b);
    }
}

} // namespace sovereign::milp
