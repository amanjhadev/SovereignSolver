#include "sovereign/qp/presolve_qp_solver.hpp"

namespace sovereign::qp {

PresolveQPSolver::PresolveQPSolver()
    : PresolveQPSolver(SolverConfig{}, presolve::PresolveConfig{}, QPConfig{}) {}

PresolveQPSolver::PresolveQPSolver(
    const SolverConfig& config,
    const presolve::PresolveConfig& presolve_config,
    const QPConfig& qp_config
) : config_(config), presolve_config_(presolve_config), qp_config_(qp_config) {
    // Empty column LP elimination & free col singleton substitution must be disabled for QP presolve
    // because quadratic terms Q_jj > 0 bound variables independently of LP bounds.
    presolve_config_.enable_empty_cols = false;
    presolve_config_.enable_free_col_singleton = false;
    presolve_config_.enable_row_singleton = false;
}

lp::SimplexResult PresolveQPSolver::solve(const QPModel& orig_qp_model) {
    // 1. Run Presolve Engine on underlying LP model
    presolve::PresolveEngine presolver(presolve_config_);
    auto presolve_res = presolver.presolve(orig_qp_model.lp_model());

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

    // 2. Construct Presolved QP Model
    QPModel presolved_qp(presolve_res.presolved_model);
    presolved_qp.lp_model().set_objective_sense(orig_qp_model.objective_sense());

    Index orig_n = orig_qp_model.num_variables();
    Index pres_n = presolve_res.presolved_model.num_variables();

    // Map Quadratic Objective Matrix Q to Presolved Variable Space
    for (const auto& entry : orig_qp_model.q_entries()) {
        Index i = entry.i;
        Index j = entry.j;
        Float64 q_val = entry.val;

        Index pres_i = -1;
        Index pres_j = -1;

        for (Index p = 0; p < pres_n; ++p) {
            Index orig_idx = presolve_res.presolved_model.get_variable(p).original_var_index;
            if (orig_idx < 0) {
                for (Index k = 0; k < orig_n; ++k) {
                    if (orig_qp_model.lp_model().get_variable(k).name == presolve_res.presolved_model.get_variable(p).name) {
                        orig_idx = k;
                        break;
                    }
                }
            }
            if (orig_idx == i) pres_i = p;
            if (orig_idx == j) pres_j = p;
        }

        if (pres_i >= 0 && pres_j >= 0) {
            presolved_qp.add_quadratic_term(pres_i, pres_j, q_val);
        }
    }

    // 3. Solve Presolved QP Model using Convex QP Active-Set Solver
    QPSolver solver(qp_config_);
    QPResult qp_res = solver.solve(presolved_qp);

    if (qp_res.status != QPStatus::Optimal) {
        lp::SimplexResult res;
        res.status = lp::SimplexStatus::NumericalFailure;
        res.message = qp_res.message;
        return res;
    }

    // 4. Map presolved QP solution back to original variables
    lp::SimplexResult final_res;
    final_res.status = lp::SimplexStatus::Optimal;
    final_res.is_primal_feasible = qp_res.is_primal_feasible;
    final_res.is_dual_feasible = qp_res.is_dual_feasible;
    final_res.iterations = qp_res.iterations;
    final_res.primal_solution.assign(orig_n, 0.0);

    for (Index p = 0; p < pres_n; ++p) {
        Index orig_idx = presolve_res.presolved_model.get_variable(p).original_var_index;
        if (orig_idx < 0) {
            for (Index k = 0; k < orig_n; ++k) {
                if (orig_qp_model.lp_model().get_variable(k).name == presolve_res.presolved_model.get_variable(p).name) {
                    orig_idx = k;
                    break;
                }
            }
        }
        if (orig_idx >= 0 && orig_idx < orig_n && p < static_cast<Index>(qp_res.primal_solution.size())) {
            final_res.primal_solution[orig_idx] = qp_res.primal_solution[p];
        }
    }

    // Fill in fixed variables from orig_qp_model bounds
    for (Index j = 0; j < orig_n; ++j) {
        const auto& var = orig_qp_model.lp_model().get_variable(j);
        if (std::abs(var.lower_bound - var.upper_bound) < 1e-9) {
            final_res.primal_solution[j] = var.lower_bound;
        }
    }

    // Evaluate exact original QP objective
    Float64 exact_obj = orig_qp_model.evaluate_objective(final_res.primal_solution);
    final_res.objective_value = exact_obj;

    return final_res;
}

} // namespace sovereign::qp
