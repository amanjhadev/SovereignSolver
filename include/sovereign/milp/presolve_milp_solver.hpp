#ifndef SOVEREIGN_MILP_PRESOLVE_MILP_SOLVER_HPP
#define SOVEREIGN_MILP_PRESOLVE_MILP_SOLVER_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include "sovereign/presolve/presolve_engine.hpp"
#include "sovereign/presolve/postsolve_engine.hpp"

namespace sovereign::milp {

class PresolveMILPSolver {
private:
    SolverConfig config_;
    presolve::PresolveConfig presolve_config_;
    MILPConfig milp_config_;

public:
    PresolveMILPSolver();
    explicit PresolveMILPSolver(
        const SolverConfig& config,
        const presolve::PresolveConfig& presolve_config = presolve::PresolveConfig{},
        const MILPConfig& milp_config = MILPConfig{}
    );

    // Solves MILPModel using Integrality-Preserving Presolve -> MILPSolver -> Postsolve
    MILPResult solve(const MILPModel& orig_milp);
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_PRESOLVE_MILP_SOLVER_HPP
