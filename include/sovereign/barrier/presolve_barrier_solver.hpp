#ifndef SOVEREIGN_BARRIER_PRESOLVE_BARRIER_SOLVER_HPP
#define SOVEREIGN_BARRIER_PRESOLVE_BARRIER_SOLVER_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/simplex_types.hpp"
#include "sovereign/presolve/presolve_engine.hpp"
#include "sovereign/presolve/postsolve_engine.hpp"
#include "sovereign/barrier/barrier_solver.hpp"

namespace sovereign::barrier {

class PresolveBarrierSolver {
private:
    SolverConfig config_;
    presolve::PresolveConfig presolve_config_;
    BarrierConfig barrier_config_;

public:
    PresolveBarrierSolver() = default;
    explicit PresolveBarrierSolver(
        const SolverConfig& config,
        const presolve::PresolveConfig& presolve_config = presolve::PresolveConfig{},
        const BarrierConfig& barrier_config = BarrierConfig{}
    );

    // Solves model using Presolve -> Barrier Engine -> Postsolve
    // Converts BarrierResult to standard SimplexResult at boundary
    lp::SimplexResult solve(const lp::LPModel& orig_model);
};

} // namespace sovereign::barrier

#endif // SOVEREIGN_BARRIER_PRESOLVE_BARRIER_SOLVER_HPP
