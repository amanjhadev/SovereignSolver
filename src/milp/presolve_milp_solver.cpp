#include "sovereign/milp/presolve_milp_solver.hpp"
#include <map>
#include <iostream>
#include <cmath>

namespace sovereign::milp {

PresolveMILPSolver::PresolveMILPSolver()
    : PresolveMILPSolver(SolverConfig{}, presolve::PresolveConfig{}, MILPConfig{}) {}

PresolveMILPSolver::PresolveMILPSolver(
    const SolverConfig& config,
    const presolve::PresolveConfig& presolve_config,
    const MILPConfig& milp_config
) : config_(config), presolve_config_(presolve_config), milp_config_(milp_config) {
    // Disable continuous-only presolve reductions that alter integer lattices
    presolve_config_.enable_free_col_singleton = false;
    presolve_config_.enable_empty_cols = false;
    presolve_config_.enable_row_singleton = false;
    presolve_config_.enable_empty_cols = false;
}

MILPResult PresolveMILPSolver::solve(const MILPModel& orig_milp) {
    // 1. Run Presolve Engine on underlying LP model
    presolve::PresolveEngine presolver(presolve_config_);
    auto presolve_res = presolver.presolve(orig_milp.lp_model());

    if (presolve_res.status == presolve::PresolveStatus::Infeasible) {
        MILPResult res;
        res.status = MILPStatus::Infeasible;
        res.message = "Presolve proved model INFEASIBLE: " + presolve_res.message;
        res.nodes_explored = 0;
        return res;
    }

    if (presolve_res.status == presolve::PresolveStatus::Unbounded) {
        MILPResult res;
        res.status = MILPStatus::Unresolved;
        res.message = "Presolve proved LP relaxation UNBOUNDED: " + presolve_res.message;
        res.nodes_explored = 0;
        return res;
    }

    // 2. Construct Presolved MILPModel
    MILPModel presolved_milp(presolve_res.presolved_model);

    Index orig_n = orig_milp.num_variables();
    Index pres_n = presolve_res.presolved_model.num_variables();

    for (Index p = 0; p < pres_n; ++p) {
        Index orig_idx = presolve_res.presolved_model.get_variable(p).original_var_index;
        if (orig_idx < 0) {
            for (Index j = 0; j < orig_n; ++j) {
                if (orig_milp.lp_model().get_variable(j).name == presolve_res.presolved_model.get_variable(p).name) {
                    orig_idx = j;
                    break;
                }
            }
        }
        if (orig_idx >= 0 && orig_idx < orig_n) {
            presolved_milp.set_variable_type(p, orig_milp.variable_type(orig_idx));
        }
    }

    // 3. Solve Presolved MILP Model
    MILPSolver solver(milp_config_);
    MILPResult milp_res = solver.solve(presolved_milp);

    if (milp_res.status != MILPStatus::Optimal) {
        return milp_res;
    }

    // 4. Map presolved MILP solution back to original variables
    MILPResult final_res = milp_res;
    final_res.primal_solution.assign(orig_n, 0.0);

    for (Index p = 0; p < pres_n; ++p) {
        Index orig_idx = presolve_res.presolved_model.get_variable(p).original_var_index;
        if (orig_idx < 0) {
            for (Index j = 0; j < orig_n; ++j) {
                if (orig_milp.lp_model().get_variable(j).name == presolve_res.presolved_model.get_variable(p).name) {
                    orig_idx = j;
                    break;
                }
            }
        }
        if (orig_idx >= 0 && orig_idx < orig_n && p < static_cast<Index>(milp_res.primal_solution.size())) {
            final_res.primal_solution[orig_idx] = milp_res.primal_solution[p];
        }
    }

    // Fill in fixed variables from orig_milp bounds
    for (Index j = 0; j < orig_n; ++j) {
        const auto& var = orig_milp.lp_model().get_variable(j);
        if (std::abs(var.lower_bound - var.upper_bound) < 1e-9) {
            final_res.primal_solution[j] = var.lower_bound;
        }
    }

    // Evaluate exact original objective
    Float64 exact_obj = orig_milp.objective_offset();
    for (Index j = 0; j < orig_n; ++j) {
        exact_obj += orig_milp.lp_model().get_variable(j).obj_coeff * final_res.primal_solution[j];
    }
    final_res.objective_value = exact_obj;

    return final_res;
}

} // namespace sovereign::milp
