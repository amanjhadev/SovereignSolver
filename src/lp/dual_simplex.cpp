#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/initial_basis.hpp"
#include "sovereign/lp/lp_validator.hpp"
#include "sovereign/lp/primal_simplex.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <iostream>

namespace sovereign::lp {

DualSimplexSolver::DualSimplexSolver(const SolverConfig& config)
    : config_(config),
      tol_primal_(config.feasibility_tolerance),
      tol_dual_(config.optimality_tolerance),
      tol_pivot_(config.pivot_tolerance),
      max_iterations_(config.max_iterations) {}

SimplexResult DualSimplexSolver::solve(const LPModel& orig_model) {
    Int64 total_iters = 0;

    // 1. Validate original model
    auto val_res = LPValidator::validate(orig_model);
    if (!val_res.is_valid) {
        SimplexResult res;
        if (val_res.is_infeasible) {
            res.status = SimplexStatus::Infeasible;
            res.message = "LPValidator proved infeasibility: " + (val_res.errors.empty() ? "Infeasible bounds" : val_res.errors[0]);
            return res;
        }
        res.status = SimplexStatus::NumericalFailure;
        res.message = "LPValidator failed: " + (val_res.errors.empty() ? "Invalid model" : val_res.errors[0]);
        return res;
    }

    // 2. Convert to standard form
    LPModel std_model = orig_model.convert_to_standard_form();

    // 3. Try constructing logical basis
    Basis initial_basis;
    Status init_status = Status::Error;
    try {
        initial_basis = InitialBasisBuilder::construct_logical_basis(std_model);
        BasisMatrix B_mat;
        init_status = InitialBasisBuilder::validate_initial_basis(std_model, initial_basis, B_mat);
    } catch (...) {
        init_status = Status::InvalidInput;
    }

    SimplexState state;
    bool need_phase1 = true;

    if (is_ok(init_status)) {
        state = SimplexState(std_model, initial_basis);
        if (is_ok(state.factorize_basis(config_)) &&
            is_ok(state.solve_primal_solution()) &&
            is_ok(state.solve_dual_solution())) {
            
            Float64 max_d_viol = 0.0;
            Index worst_col = -1;
            Float64 max_p_viol = 0.0;
            Index worst_row = -1;
            bool p_feas = state.check_primal_feasibility(tol_primal_, max_p_viol, worst_row);
            bool d_feas = state.check_dual_feasibility(tol_dual_, max_d_viol, worst_col);

            if (p_feas && d_feas) {
                // Initial basis is already Optimal!
                return recover_solution(orig_model, std_model, state, SimplexStatus::Optimal, 0, "Initial basis is optimal");
            }

            if (d_feas) {
                // Initial basis is Dual Feasible -> proceed directly to Phase II Dual Simplex!
                need_phase1 = false;
            }
        }
    }

    SimplexStatus status = SimplexStatus::NotSolved;

    if (need_phase1) {
        status = solve_phase_1(std_model, state, total_iters);
        if (status != SimplexStatus::Optimal) { // Phase I failed or found infeasible
            return recover_solution(orig_model, std_model, state, status, total_iters, "Phase I completed");
        }
    }

    // Phase II: Choose Dual or Primal Simplex based on starting dual feasibility
    Float64 dummy_viol = 0.0;
    Index dummy_idx = -1;
    bool start_d_feas = state.check_dual_feasibility(tol_dual_, dummy_viol, dummy_idx);

    if (start_d_feas) {
        status = solve_phase_2(state, total_iters);
    } else {
        PrimalSimplexSolver p2_solver(config_);
        status = p2_solver.solve(state, total_iters);
    }

    return recover_solution(orig_model, std_model, state, status, total_iters, "Phase II completed");
}

SimplexStatus DualSimplexSolver::solve_phase_1(const LPModel& std_model, SimplexState& state, Int64& total_iters) {
    Index m = std_model.num_constraints();
    Index n = std_model.num_variables();

    // Create Phase I auxiliary LP model
    LPModel phase1_model("phase1_aux", ObjectiveSense::Minimize);

    // Copy standard form variables with 0 obj coeff
    for (Index j = 0; j < n; ++j) {
        const auto& var = std_model.get_variable(j);
        phase1_model.add_variable(var.name, var.lower_bound, var.upper_bound, 0.0, var.type);
    }

    const auto& csc = std_model.get_csc_matrix();
    std::vector<Index> artificial_cols;

    // Add constraints and artificial variables
    for (Index i = 0; i < m; ++i) {
        const auto& c = std_model.get_constraint(i);
        std::vector<Index> row_cols;
        std::vector<Float64> row_vals;

        for (Index j = 0; j < n; ++j) {
            auto r_indices = csc.get_col_row_indices(j);
            auto c_vals = csc.get_col_values(j);
            for (std::size_t k = 0; k < r_indices.size(); ++k) {
                if (r_indices[k] == i) {
                    row_cols.push_back(j);
                    row_vals.push_back(c_vals[k]);
                }
            }
        }

        Float64 sign = (c.rhs >= 0.0) ? 1.0 : -1.0;
        Float64 rhs_val = (c.rhs >= 0.0) ? c.rhs : -c.rhs;

        Index art_col = phase1_model.add_variable("art_" + std::to_string(i), 0.0, INFINITY_VAL, 1.0);
        artificial_cols.push_back(art_col);

        if (sign < 0.0) {
            for (auto& val : row_vals) val = -val;
        }

        row_cols.push_back(art_col);
        row_vals.push_back(1.0);

        phase1_model.add_constraint("c_" + std::to_string(i), ConstraintSense::Equal, rhs_val, row_cols, row_vals);
    }

    // Construct identity artificial basis for Phase I
    Basis phase1_basis(m, phase1_model.num_variables());
    for (Index i = 0; i < m; ++i) {
        phase1_basis.set_basic(artificial_cols[i], i);
    }
    for (Index j = 0; j < phase1_model.num_variables(); ++j) {
        if (phase1_basis.get_status(j) != BasisStatus::Basic) {
            const auto& v = phase1_model.get_variable(j);
            if (v.lower_bound > -INFINITY_VAL) {
                phase1_basis.set_nonbasic(j, BasisStatus::AtLower);
            } else if (v.upper_bound < INFINITY_VAL) {
                phase1_basis.set_nonbasic(j, BasisStatus::AtUpper);
            } else {
                phase1_basis.set_nonbasic(j, BasisStatus::Free);
            }
        }
    }
    phase1_basis.rebuild_position_maps();

    SimplexState p1_state(phase1_model, phase1_basis);
    if (!is_ok(p1_state.factorize_basis(config_))) {
        return SimplexStatus::NumericalFailure;
    }

    p1_state.solve_primal_solution();
    p1_state.solve_dual_solution();

    // Solve Phase I auxiliary problem using Primal Simplex (primal feasible starting point)
    PrimalSimplexSolver p1_solver(config_);
    SimplexStatus p1_status = p1_solver.solve(p1_state, total_iters);
    if (p1_status != SimplexStatus::Optimal && p1_status != SimplexStatus::NotSolved) {
        return p1_status;
    }

    Float64 p1_obj = p1_state.compute_objective_value();
    if (p1_obj > tol_primal_) {
        return SimplexStatus::Infeasible;
    }

    // Phase I succeeded: drive any basic artificial variables out of basis safely
    Basis final_basis(m, n);
    std::set<Index> assigned_basic_cols;

    for (Index i = 0; i < m; ++i) {
        Index col = p1_state.basis().get_basic_var(i);
        if (col >= n) { // Artificial variable basic at 0
            Index replacement_col = -1;
            const auto& p1_csc = phase1_model.get_csc_matrix();
            for (Index j = 0; j < n; ++j) {
                if (assigned_basic_cols.find(j) == assigned_basic_cols.end() &&
                    p1_state.basis().get_status(j) != BasisStatus::Basic) {
                    auto r_idx = p1_csc.get_col_row_indices(j);
                    auto c_v = p1_csc.get_col_values(j);
                    for (std::size_t k = 0; k < r_idx.size(); ++k) {
                        if (r_idx[k] == i && std::abs(c_v[k]) > tol_pivot_) {
                            replacement_col = j;
                            break;
                        }
                    }
                }
                if (replacement_col != -1) break;
            }
            if (replacement_col != -1) {
                col = replacement_col;
            } else {
                return SimplexStatus::NumericalFailure; // Cannot construct clean basis
            }
        }
        final_basis.set_basic(col, i);
        assigned_basic_cols.insert(col);
    }

    for (Index j = 0; j < n; ++j) {
        if (final_basis.get_status(j) != BasisStatus::Basic) {
            BasisStatus st = p1_state.basis().get_status(j);
            if (st == BasisStatus::Basic) st = BasisStatus::AtLower;
            final_basis.set_nonbasic(j, st);
        }
    }
    final_basis.rebuild_position_maps();

    state = SimplexState(std_model, final_basis);
    if (!is_ok(state.factorize_basis(config_))) {
        return SimplexStatus::NumericalFailure;
    }
    state.solve_primal_solution();
    state.solve_dual_solution();

    return SimplexStatus::Optimal;
}

SimplexStatus DualSimplexSolver::solve_phase_2(SimplexState& state, Int64& total_iters) {
    DevexPricing devex(state.num_rows());
    DualRatioTest ratio_tester(tol_primal_, tol_dual_, tol_pivot_);

    while (total_iters < max_iterations_) {
        // Factorize basis & solve primal/dual
        if (!is_ok(state.factorize_basis(config_)) ||
            !is_ok(state.solve_primal_solution()) ||
            !is_ok(state.solve_dual_solution())) {
            return SimplexStatus::NumericalFailure;
        }

        // 1. Check Primal Feasibility
        Float64 max_p_viol = 0.0;
        Index worst_row = -1;
        if (state.check_primal_feasibility(tol_primal_, max_p_viol, worst_row)) {
            return SimplexStatus::Optimal;
        }

        // 2. Select Leaving Variable using Devex pricing
        Float64 max_weighted_viol = 0.0;
        Index leaving_slot = devex.select_leaving_variable(state, tol_primal_, max_weighted_viol);
        if (leaving_slot == -1) {
            leaving_slot = worst_row;
        }

        if (leaving_slot < 0 || leaving_slot >= state.num_rows()) {
            return SimplexStatus::Optimal;
        }

        // 3. BTRAN for row vector y_p and row alphas
        std::vector<Float64> y_p(state.num_rows(), 0.0);
        if (!is_ok(state.compute_btran_row(leaving_slot, y_p))) {
            return SimplexStatus::NumericalFailure;
        }

        std::vector<Float64> alphas;
        state.compute_row_alphas(y_p, alphas);

        // 4. Dual Ratio Test for Entering Variable
        auto ratio_res = ratio_tester.select_entering_variable_harris(state, leaving_slot, alphas);
        if (!ratio_res.valid) {
            return SimplexStatus::Infeasible;
        }

        Index entering_col = ratio_res.entering_col;

        // 5. FTRAN for entering column direction d
        std::vector<Float64> d(state.num_rows(), 0.0);
        if (!is_ok(state.compute_ftran_direction(entering_col, d))) {
            return SimplexStatus::NumericalFailure;
        }

        // 6. BFRT Check for Boxed Variable Flip
        const auto& ent_var = state.std_model().get_variable(entering_col);
        BasisStatus ent_status = state.basis().get_status(entering_col);
        Float64 d_q = state.reduced_costs()[entering_col];
        bool is_dual_valid_flip = (ent_status == BasisStatus::AtLower && d_q <= tol_dual_) ||
                                  (ent_status == BasisStatus::AtUpper && d_q >= -tol_dual_);
        if (is_dual_valid_flip && ent_var.lower_bound > -INFINITY_VAL && ent_var.upper_bound < INFINITY_VAL) {
            Float64 delta_x = ent_var.upper_bound - ent_var.lower_bound;
            Float64 alpha_p = alphas[entering_col];
            if (std::abs(alpha_p) > tol_pivot_) {
                Float64 delta_xb = delta_x * alpha_p;
                Index leaving_col = state.basis().get_basic_var(leaving_slot);
                const auto& var_l = state.std_model().get_variable(leaving_col);
                Float64 val_l = state.x_B()[leaving_slot];

                // If flip keeps leaving row within bounds, execute bound flip without basis change!
                Float64 new_val_l = val_l - delta_xb;
                if (new_val_l >= var_l.lower_bound - tol_primal_ && new_val_l <= var_l.upper_bound + tol_primal_) {
                    BasisStatus flip_status = (ent_status == BasisStatus::AtLower) ? BasisStatus::AtUpper : BasisStatus::AtLower;
                    state.basis().set_nonbasic(entering_col, flip_status);
                    total_iters++;
                    state.increment_iterations();
                    continue; // Skip basis pivot, recompute b_eff directly!
                }
            }
        }

        // 7. Basis Pivot Step & MANDATORY POST-PIVOT REFACTORIZATION
        Index leaving_col = state.basis().get_basic_var(leaving_slot);
        const auto& var_l = state.std_model().get_variable(leaving_col);
        Float64 val_l = state.x_B()[leaving_slot];

        BasisStatus leaving_new_status = BasisStatus::AtLower;
        if (val_l > var_l.upper_bound) {
            leaving_new_status = BasisStatus::AtUpper;
        }

        state.basis().pivot(entering_col, leaving_slot, leaving_new_status);
        devex.update_weights(leaving_slot, d);

        // Immediate post-pivot refactorization rule
        if (!is_ok(state.factorize_basis(config_))) {
            return SimplexStatus::NumericalFailure;
        }

        total_iters++;
        state.increment_iterations();
    }

    return SimplexStatus::IterationLimit;
}

SimplexResult DualSimplexSolver::recover_solution(
    const LPModel& orig_model,
    const LPModel& std_model,
    const SimplexState& state,
    SimplexStatus status,
    Int64 total_iters,
    const std::string& msg
) const {
    SimplexResult res;
    res.status = status;
    res.iterations = total_iters;
    res.message = msg;

    if (status == SimplexStatus::Optimal) {
        SimplexState mut_state = state;
        mut_state.factorize_basis(config_);
        mut_state.solve_primal_solution();
        mut_state.solve_dual_solution();

        auto x_std = mut_state.x_all();
        res.primal_solution = std_model.standard_form_map().recover_original_solution(x_std);
        res.dual_solution.assign(mut_state.y().begin(), mut_state.y().end());
        res.reduced_costs.assign(mut_state.reduced_costs().begin(), mut_state.reduced_costs().end());

        res.basis = mut_state.basis();
        Float64 obj = orig_model.objective_offset();
        for (Index j = 0; j < orig_model.num_variables(); ++j) {
            obj += orig_model.get_variable(j).obj_coeff * res.primal_solution[j];
        }
        res.objective_value = obj;

        // Original Residual Verification
        const auto& orig_csc = orig_model.get_csc_matrix();
        Float64 max_row_res = 0.0;

        for (Index i = 0; i < orig_model.num_constraints(); ++i) {
            const auto& c = orig_model.get_constraint(i);
            Float64 row_val = 0.0;

            for (Index j = 0; j < orig_model.num_variables(); ++j) {
                auto r_idx = orig_csc.get_col_row_indices(j);
                auto c_v = orig_csc.get_col_values(j);
                for (std::size_t k = 0; k < r_idx.size(); ++k) {
                    if (r_idx[k] == i) {
                        row_val += c_v[k] * res.primal_solution[j];
                    }
                }
            }

            Float64 r_viol = 0.0;
            if (c.sense == ConstraintSense::LessEqual) {
                r_viol = std::max(0.0, row_val - c.rhs);
            } else if (c.sense == ConstraintSense::GreaterEqual) {
                r_viol = std::max(0.0, c.lhs - row_val);
            } else if (c.sense == ConstraintSense::Equal) {
                r_viol = std::abs(row_val - c.rhs);
            } else if (c.sense == ConstraintSense::Range) {
                r_viol = std::max(0.0, std::max(c.lhs - row_val, row_val - c.rhs));
            }

            max_row_res = std::max(max_row_res, r_viol);
        }

        res.max_primal_residual = max_row_res;
        res.is_primal_feasible = (max_row_res <= tol_primal_ * 100.0);

        Float64 max_d_viol = 0.0;
        Index worst_col = -1;
        res.is_dual_feasible = mut_state.check_dual_feasibility(tol_dual_ * 1000.0, max_d_viol, worst_col);
        res.max_dual_residual = max_d_viol;
    }

    return res;
}

} // namespace sovereign::lp
