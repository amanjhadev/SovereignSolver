#include "sovereign/barrier/presolve_barrier_solver.hpp"

namespace sovereign::barrier {

PresolveBarrierSolver::PresolveBarrierSolver(
    const SolverConfig& config,
    const presolve::PresolveConfig& presolve_config,
    const BarrierConfig& barrier_config
) : config_(config), presolve_config_(presolve_config), barrier_config_(barrier_config) {}

lp::SimplexResult PresolveBarrierSolver::solve(const lp::LPModel& orig_model) {
    // 1. Run Presolve Engine
    presolve::PresolveEngine presolver(presolve_config_);
    auto presolve_res = presolver.presolve(orig_model);

    if (presolve_res.status == presolve::PresolveStatus::Infeasible) {
        lp::SimplexResult res;
        res.status = lp::SimplexStatus::Infeasible;
        res.message = "Presolve proved model INFEASIBLE: " + presolve_res.message;
        res.iterations = 0;
        return res;
    }

    if (presolve_res.status == presolve::PresolveStatus::Unbounded) {
        lp::SimplexResult res;
        res.status = lp::SimplexStatus::Unbounded;
        res.message = "Presolve proved model UNBOUNDED: " + presolve_res.message;
        res.iterations = 0;
        return res;
    }

    // 2. Run Barrier Solver on presolved model
    BarrierSolver b_solver(barrier_config_);
    BarrierResult b_res = b_solver.solve(presolve_res.presolved_model);

    // 3. Convert BarrierResult to standard SimplexResult at boundary
    lp::SimplexResult presolved_simplex_res;
    presolved_simplex_res.objective_value = b_res.objective_value;
    presolved_simplex_res.primal_solution = b_res.primal_solution;
    presolved_simplex_res.dual_solution = b_res.dual_solution;
    presolved_simplex_res.iterations = b_res.iterations;
    presolved_simplex_res.max_primal_residual = b_res.primal_residual;
    presolved_simplex_res.max_dual_residual = b_res.dual_residual;
    presolved_simplex_res.is_primal_feasible = b_res.is_primal_feasible;
    presolved_simplex_res.is_dual_feasible = b_res.is_dual_feasible;
    presolved_simplex_res.message = b_res.message;

    switch (b_res.status) {
        case BarrierStatus::Optimal:
            presolved_simplex_res.status = lp::SimplexStatus::Optimal;
            break;
        case BarrierStatus::PrimalInfeasible:
            presolved_simplex_res.status = lp::SimplexStatus::Infeasible;
            break;
        case BarrierStatus::DualInfeasible:
            presolved_simplex_res.status = lp::SimplexStatus::Unbounded;
            break;
        case BarrierStatus::IterationLimit:
            presolved_simplex_res.status = lp::SimplexStatus::IterationLimit;
            break;
        case BarrierStatus::NumericalFailure:
        default:
            presolved_simplex_res.status = lp::SimplexStatus::NumericalFailure;
            break;
    }

    if (presolved_simplex_res.status != lp::SimplexStatus::Optimal) {
        return presolved_simplex_res;
    }

    // 4. Postsolve to recover original model solution
    presolve::PostsolveEngine postsolver(config_.feasibility_tolerance);
    lp::SimplexResult final_res = postsolver.postsolve(orig_model, presolve_res.presolved_model, presolved_simplex_res, presolve_res.stack);

    return final_res;
}

} // namespace sovereign::barrier
