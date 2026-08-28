#ifndef SOVEREIGN_BARRIER_BARRIER_SOLVER_HPP
#define SOVEREIGN_BARRIER_BARRIER_SOLVER_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/barrier/barrier_types.hpp"
#include "sovereign/barrier/sparse_cholesky.hpp"

namespace sovereign::barrier {

class BarrierSolver {
private:
    BarrierConfig config_;

public:
    BarrierSolver() = default;
    explicit BarrierSolver(const BarrierConfig& config) : config_(config) {}

    // Solves LPModel using Infeasible-Start Primal-Dual Mehrotra Interior-Point Method
    BarrierResult solve(const lp::LPModel& model) const;
};

} // namespace sovereign::barrier

#endif // SOVEREIGN_BARRIER_BARRIER_SOLVER_HPP
