#ifndef SOVEREIGN_MILP_BRANCHING_HPP
#define SOVEREIGN_MILP_BRANCHING_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_types.hpp"
#include "sovereign/milp/bb_node.hpp"
#include <span>
#include <vector>

namespace sovereign::milp {

class BranchTranslationLayer {
public:
    BranchTranslationLayer() = default;

    // Translates original variable branch x_orig <= floor(x*) or x_orig >= ceil(x*)
    // into either column bound changes or linear branch constraints.
    void create_child_branch_constraints(
        const MILPModel& model,
        Index orig_idx,
        Float64 val,
        BBNode& left_child,
        BBNode& right_child
    ) const;
};

bool is_integral(Float64 val, Float64 tol_integrality) noexcept;

bool check_solution_integrality(
    std::span<const Float64> sol,
    std::span<const Index> integer_var_indices,
    Float64 tol_integrality
);

Index select_branching_variable(
    std::span<const Float64> sol,
    std::span<const Index> integer_var_indices,
    BranchingRule rule,
    Float64 tol_integrality
);

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_BRANCHING_HPP
