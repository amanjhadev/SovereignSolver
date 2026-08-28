#ifndef SOVEREIGN_LP_SIMPLEX_TYPES_HPP
#define SOVEREIGN_LP_SIMPLEX_TYPES_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/basis.hpp"
#include <string>
#include <vector>

namespace sovereign::lp {

enum class SimplexStatus : std::uint8_t {
    NotSolved        = 0,
    Optimal          = 1,
    Infeasible       = 2,
    Unbounded        = 3,
    IterationLimit   = 4,
    NumericalFailure = 5
};

enum class PricingStrategy : std::uint8_t {
    Bland   = 0,
    Devex   = 1
};

struct SimplexResult {
    SimplexStatus status{SimplexStatus::NotSolved};
    Float64 objective_value{0.0};
    std::vector<Float64> primal_solution;   // Recovered for original LP
    std::vector<Float64> dual_solution;     // Dual variables y for standard LP
    std::vector<Float64> reduced_costs;      // Reduced costs for standard LP
    Basis basis;                            // Optimal basis
    Int64 iterations{0};
    Float64 max_primal_residual{0.0};
    Float64 max_dual_residual{0.0};
    bool is_primal_feasible{false};
    bool is_dual_feasible{false};
    std::string message;
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_SIMPLEX_TYPES_HPP
