#ifndef SOVEREIGN_BARRIER_BARRIER_TYPES_HPP
#define SOVEREIGN_BARRIER_BARRIER_TYPES_HPP

#include "sovereign/core/types.hpp"
#include <string>
#include <vector>

namespace sovereign::barrier {

enum class BarrierStatus : std::uint8_t {
    NotSolved        = 0,
    Optimal          = 1,
    PrimalInfeasible = 2,
    DualInfeasible   = 3,
    IterationLimit   = 4,
    NumericalFailure = 5
};

struct BarrierResult {
    BarrierStatus status{BarrierStatus::NotSolved};
    Float64 objective_value{0.0};
    std::vector<Float64> primal_solution; // x
    std::vector<Float64> dual_solution;   // y
    std::vector<Float64> dual_slacks;     // s
    Int64 iterations{0};
    Float64 primal_residual{0.0};
    Float64 dual_residual{0.0};
    Float64 duality_gap{0.0};
    Float64 mu{0.0};
    bool is_primal_feasible{false};
    bool is_dual_feasible{false};
    std::string message;
};

struct BarrierConfig {
    Int64 max_iterations{200};
    Float64 tol_primal{1e-6};
    Float64 tol_dual{1e-6};
    Float64 tol_gap{1e-6};
    Float64 tol_mu{1e-8};
    Float64 step_safety_factor{0.995};
    Float64 min_pivot{1e-12};
    Float64 initial_regularization{1e-10};
    Float64 max_regularization{1e-4};
};

} // namespace sovereign::barrier

#endif // SOVEREIGN_BARRIER_BARRIER_TYPES_HPP
