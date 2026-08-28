#include "sovereign/lp/presolve_solver.hpp"

namespace sovereign::lp {

PresolveSolver::PresolveSolver(const SolverConfig& config, const presolve::PresolveConfig& presolve_config)
    : config_(config), presolve_config_(presolve_config) {}

SimplexResult PresolveSolver::solve(const LPModel& orig_model) {
    // 1. Run Presolve Engine
    presolve::PresolveEngine presolver(presolve_config_);
    auto presolve_res = presolver.presolve(orig_model);

    if (presolve_res.status == presolve::PresolveStatus::Infeasible) {
        SimplexResult res;
        res.status = SimplexStatus::Infeasible;
        res.message = "Presolve proved model INFEASIBLE: " + presolve_res.message;
        res.iterations = 0;
        return res;
    }

    if (presolve_res.status == presolve::PresolveStatus::Unbounded) {
        SimplexResult res;
        res.status = SimplexStatus::Unbounded;
        res.message = "Presolve proved model UNBOUNDED: " + presolve_res.message;
        res.iterations = 0;
        return res;
    }

    // 2. Solve presolved model with Dual Simplex Engine
    SimplexResult presolved_solve_res;
    if (presolve_res.presolved_model.num_constraints() == 0) {
        presolved_solve_res.status = SimplexStatus::Optimal;
        presolved_solve_res.iterations = 0;
        Index n_pres = presolve_res.presolved_model.num_variables();
        presolved_solve_res.primal_solution.resize(n_pres, 0.0);
        bool is_min = (orig_model.objective_sense() == ObjectiveSense::Minimize);
        for (Index j = 0; j < n_pres; ++j) {
            const auto& v = presolve_res.presolved_model.get_variable(j);
            Float64 c = v.obj_coeff;
            if (is_min) {
                presolved_solve_res.primal_solution[j] = (c > 0) ? v.lower_bound : ((c < 0) ? v.upper_bound : ((v.lower_bound > -INFINITY_VAL / 2.0) ? v.lower_bound : 0.0));
            } else {
                presolved_solve_res.primal_solution[j] = (c > 0) ? v.upper_bound : ((c < 0) ? v.lower_bound : ((v.lower_bound > -INFINITY_VAL / 2.0) ? v.lower_bound : 0.0));
            }
        }
    } else {
        DualSimplexSolver solver(config_);
        presolved_solve_res = solver.solve(presolve_res.presolved_model);
    }

    if (presolved_solve_res.status != SimplexStatus::Optimal) {
        return presolved_solve_res;
    }

    // 3. Postsolve to recover original model solution
    presolve::PostsolveEngine postsolver(config_.feasibility_tolerance);
    SimplexResult final_res = postsolver.postsolve(orig_model, presolve_res.presolved_model, presolved_solve_res, presolve_res.stack);

    return final_res;
}

} // namespace sovereign::lp
