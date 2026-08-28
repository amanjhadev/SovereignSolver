#ifndef SOVEREIGN_LP_LP_SOLUTION_HPP
#define SOVEREIGN_LP_LP_SOLUTION_HPP

#include "sovereign/core/types.hpp"
#include <vector>

namespace sovereign::lp {

enum class FeasibilityStatus : std::uint8_t {
    NotSolved      = 0,
    Optimal         = 1,
    PrimalFeasible  = 2,
    DualFeasible    = 3,
    Infeasible      = 4,
    Unbounded       = 5,
    NumericalError  = 6,
    MaxIters        = 7
};

struct LPSolution {
    FeasibilityStatus status{FeasibilityStatus::NotSolved};

    std::vector<Float64> primal_values;  // Size: num_variables (x)
    std::vector<Float64> dual_values;    // Size: num_constraints (y / pi)
    std::vector<Float64> reduced_costs;  // Size: num_variables (d = c - A^T y)

    Float64 primal_objective{0.0};
    Float64 dual_objective{0.0};

    Int32 iteration_count{0};
    Int32 refactorization_count{0};

    void reset() {
        status = FeasibilityStatus::NotSolved;
        primal_values.clear();
        dual_values.clear();
        reduced_costs.clear();
        primal_objective = 0.0;
        dual_objective = 0.0;
        iteration_count = 0;
        refactorization_count = 0;
    }

    bool is_optimal() const noexcept { return status == FeasibilityStatus::Optimal; }
    bool is_infeasible() const noexcept { return status == FeasibilityStatus::Infeasible; }
    bool is_unbounded() const noexcept { return status == FeasibilityStatus::Unbounded; }
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_LP_SOLUTION_HPP
