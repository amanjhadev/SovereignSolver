#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/initial_basis.hpp"
#include "sovereign/lp/lp_validator.hpp"
#include "sovereign/lp/primal_simplex.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <chrono>

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
    LPModel phase2_model = std_model;

    if (need_phase1) {
        status = solve_phase_1(std_model, phase2_model, state, total_iters);
        if (status != SimplexStatus::Optimal) { // Phase I failed or found infeasible
            return recover_solution(orig_model, std_model, state, status, total_iters, "Phase I completed");
        }
    }

    // Phase II: If Phase I was run, use Primal Simplex from the primal-feasible Phase I output
    if (need_phase1) {
        PrimalSimplexSolver p2_solver(config_);
        status = p2_solver.solve(state, total_iters);
    } else {
        Float64 dummy_viol = 0.0;
        Index dummy_idx = -1;
        bool start_d_feas = state.check_dual_feasibility(tol_dual_, dummy_viol, dummy_idx);

        if (start_d_feas) {
            status = solve_phase_2(state, total_iters);
        } else {
            PrimalSimplexSolver p2_solver(config_);
            status = p2_solver.solve(state, total_iters);
        }
    }

    return recover_solution(orig_model, std_model, state, status, total_iters, "Phase II completed");
}

SimplexResult DualSimplexSolver::solve(const LPModel& orig_model, const Basis& warm_start_basis) {
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

    LPModel std_model = orig_model.convert_to_standard_form();
    if (warm_start_basis.num_rows() == std_model.num_constraints() &&
        warm_start_basis.num_cols() == std_model.num_variables()) {
        try {
            SimplexState state(std_model, warm_start_basis);
            if (is_ok(state.factorize_basis(config_)) &&
                is_ok(state.solve_primal_solution()) &&
                is_ok(state.solve_dual_solution())) {

                Float64 dummy_viol = 0.0;
                Index dummy_idx = -1;
                bool d_feas = state.check_dual_feasibility(tol_dual_, dummy_viol, dummy_idx);
                if (d_feas) {
                    Float64 p_viol = 0.0;
                    Index p_idx = -1;
                    bool p_feas = state.check_primal_feasibility(tol_primal_, p_viol, p_idx);
                    if (p_feas) {
                        return recover_solution(orig_model, std_model, state, SimplexStatus::Optimal, 0, "Warm start already optimal");
                    }
                    Int64 total_iters = 0;
                    SimplexStatus p2_status = solve_phase_2(state, total_iters);
                    if (p2_status == SimplexStatus::Optimal || p2_status == SimplexStatus::Infeasible) {
                        return recover_solution(orig_model, std_model, state, p2_status, total_iters, "Warm start Dual Simplex Phase II");
                    }
                }
            }
        } catch (...) {
            // Fallback to clean solve below
        }
    }
    return solve(orig_model);
}

SimplexStatus DualSimplexSolver::solve_phase_1(const LPModel& std_model, LPModel& phase2_model, SimplexState& state, Int64& total_iters) {
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
    std::vector<Index> art_vars;
    Basis phase1_basis(m, n + m);

    for (Index i = 0; i < m; ++i) {
        const auto& constr = std_model.get_constraint(i);
        Index art_idx = phase1_model.add_variable("art_" + std::to_string(i), 0.0, INFINITY_VAL, 1.0, VariableType::Continuous);
        art_vars.push_back(art_idx);

        std::vector<Index> row_cols;
        std::vector<Float64> row_vals;
        for (Index j = 0; j < n; ++j) {
            auto r_idx = csc.get_col_row_indices(j);
            auto c_v = csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                if (r_idx[k] == i) {
                    row_cols.push_back(j);
                    row_vals.push_back(c_v[k]);
                }
            }
        }
        row_cols.push_back(art_idx);
        row_vals.push_back(1.0);

        phase1_model.add_constraint(constr.name, ConstraintSense::Equal, constr.rhs, row_cols, row_vals);
        phase1_basis.set_basic(art_idx, i);
    }

    for (Index j = 0; j < n; ++j) {
        const auto& v = std_model.get_variable(j);
        if (v.lower_bound > -INFINITY_VAL / 2.0) {
            phase1_basis.set_nonbasic(j, BasisStatus::AtLower);
        } else if (v.upper_bound < INFINITY_VAL / 2.0) {
            phase1_basis.set_nonbasic(j, BasisStatus::AtUpper);
        } else {
            phase1_basis.set_nonbasic(j, BasisStatus::Free);
        }
    }

    SimplexState p1_state(phase1_model, phase1_basis);
    if (!is_ok(p1_state.factorize_basis(config_))) {
        return SimplexStatus::NumericalFailure;
    }

    p1_state.solve_primal_solution();
    p1_state.solve_dual_solution();

    // Solve Phase I auxiliary problem using Primal Simplex
    PrimalSimplexSolver p1_solver(config_);
    SimplexStatus p1_status = p1_solver.solve(p1_state, total_iters);
    if (p1_status != SimplexStatus::Optimal && p1_status != SimplexStatus::NotSolved) {
        return p1_status;
    }

    Float64 p1_obj = p1_state.compute_objective_value();
    if (p1_obj > tol_primal_) {
        return SimplexStatus::Infeasible;
    }

    // Phase I succeeded: drive basic artificial variables out of basis safely using FTRAN pivot checks
    for (Index i = 0; i < m; ++i) {
        Index col = p1_state.basis().get_basic_var(i);
        if (col >= n) { // Artificial variable basic at 0
            Float64 max_pivot = 0.0;
            Index best_cand = -1;
            std::vector<Float64> best_d(m, 0.0);

            for (Index j = 0; j < n; ++j) {
                if (p1_state.basis().get_status(j) == BasisStatus::Basic) continue;

                std::vector<Float64> d(m, 0.0);
                Status ftran_st = p1_state.compute_ftran_direction(j, d);
                if (is_ok(ftran_st)) {
                    Float64 pivot_mag = std::abs(d[i]);
                    if (pivot_mag > tol_pivot_ && pivot_mag > max_pivot) {
                        max_pivot = pivot_mag;
                        best_cand = j;
                        best_d = d;
                    }
                }
            }

            if (best_cand != -1) {
                p1_state.basis().pivot(best_cand, i, BasisStatus::AtLower);
                Status up_st = p1_state.basis_matrix().update_pivot(i, best_d, tol_pivot_);
                if (!is_ok(up_st) || p1_state.basis_matrix().should_refactorize()) {
                    p1_state.factorize_basis(config_);
                }
            }
        }
    }

    bool has_artificial_basic = false;
    for (Index i = 0; i < m; ++i) {
        if (p1_state.basis().get_basic_var(i) >= n) {
            has_artificial_basic = true;
            break;
        }
    }

    if (!has_artificial_basic && m <= n) {
        Basis final_basis(m, n);
        for (Index i = 0; i < m; ++i) {
            final_basis.set_basic(p1_state.basis().get_basic_var(i), i);
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
    } else {
        phase2_model = phase1_model;
        for (Index j = 0; j < n; ++j) {
            phase2_model.get_variable(j).obj_coeff = std_model.get_variable(j).obj_coeff;
        }
        for (Index i = 0; i < m; ++i) {
            Index art_col = n + i;
            if (p1_state.basis().get_status(art_col) == BasisStatus::Basic) {
                phase2_model.get_variable(art_col).obj_coeff = 0.0;
            } else {
                phase2_model.get_variable(art_col).obj_coeff = 1e6; // Big-M penalty prevents non-basic artificials from entering
            }
        }

        state = SimplexState(phase2_model, p1_state.basis());
        if (!is_ok(state.factorize_basis(config_))) {
            return SimplexStatus::NumericalFailure;
        }
        state.solve_primal_solution();
        state.solve_dual_solution();
        return SimplexStatus::Optimal;
    }
}

SimplexStatus DualSimplexSolver::solve_phase_2(SimplexState& state, Int64& total_iters) {
    DevexPricing devex(state.num_rows());
    DualRatioTest ratio_tester(tol_primal_, tol_dual_, tol_pivot_);

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!state.basis_matrix().is_factored()) {
        if (!is_ok(state.factorize_basis(config_))) {
            return SimplexStatus::NumericalFailure;
        }
    }

    while (total_iters < max_iterations_) {
        auto now_time = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(now_time - start_time).count() > config_.max_time_sec) {
            return SimplexStatus::TimeLimit;
        }

        // Refactorize if update stack is full or un-factored
        if (state.basis_matrix().should_refactorize()) {
            if (!is_ok(state.factorize_basis(config_))) {
                return SimplexStatus::NumericalFailure;
            }
        }

        if (!is_ok(state.solve_primal_solution()) ||
            !is_ok(state.solve_dual_solution())) {
            // Retry with fresh full refactorization if solve fails
            if (!is_ok(state.factorize_basis(config_)) ||
                !is_ok(state.solve_primal_solution()) ||
                !is_ok(state.solve_dual_solution())) {
                return SimplexStatus::NumericalFailure;
            }
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

        // 7. Basis Pivot Step & Product-Form Eta Basis Update
        Index leaving_col = state.basis().get_basic_var(leaving_slot);
        const auto& var_l = state.std_model().get_variable(leaving_col);
        Float64 val_l = state.x_B()[leaving_slot];

        BasisStatus leaving_new_status = BasisStatus::AtLower;
        if (val_l > var_l.upper_bound) {
            leaving_new_status = BasisStatus::AtUpper;
        }

        state.basis().pivot(entering_col, leaving_slot, leaving_new_status);
        devex.update_weights(leaving_slot, d);

        // Apply Product-Form Eta Basis Update
        Status up_st = state.basis_matrix().update_pivot(leaving_slot, d, tol_pivot_);
        if (!is_ok(up_st)) {
            // Instability detected -> trigger full refactorization
            if (!is_ok(state.factorize_basis(config_))) {
                return SimplexStatus::NumericalFailure;
            }
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

        auto x_all = mut_state.x_all();
        std::vector<Float64> x_std(x_all.begin(), x_all.begin() + std::min(x_all.size(), static_cast<std::size_t>(std_model.num_variables())));
        res.primal_solution = std_model.standard_form_map().recover_original_solution(x_std);
        res.dual_solution.assign(mut_state.y().begin(), mut_state.y().end());
        res.reduced_costs.assign(mut_state.reduced_costs().begin(), mut_state.reduced_costs().end());

        Float64 obj_val = orig_model.objective_offset();
        for (Index j = 0; j < orig_model.num_variables(); ++j) {
            obj_val += orig_model.get_variable(j).obj_coeff * res.primal_solution[j];
        }
        res.objective_value = obj_val;

        if (mut_state.basis().num_cols() > std_model.num_variables()) {
            Index m = std_model.num_constraints();
            Index n = std_model.num_variables();
            if (m <= n) {
                // Ensure all artificial variables are driven out with valid FTRAN pivots
                for (Index i = 0; i < m; ++i) {
                    Index col = mut_state.basis().get_basic_var(i);
                    if (col >= n) {
                        Float64 max_pivot = 0.0;
                        Index best_cand = -1;
                        std::vector<Float64> best_d(m, 0.0);
                        for (Index j = 0; j < n; ++j) {
                            if (mut_state.basis().get_status(j) == BasisStatus::Basic) continue;
                            std::vector<Float64> d(m, 0.0);
                            if (is_ok(mut_state.compute_ftran_direction(j, d))) {
                                Float64 pivot_mag = std::abs(d[i]);
                                if (pivot_mag > tol_pivot_ && pivot_mag > max_pivot) {
                                    max_pivot = pivot_mag;
                                    best_cand = j;
                                    best_d = d;
                                }
                            }
                        }
                        if (best_cand != -1) {
                            mut_state.basis().pivot(best_cand, i, BasisStatus::AtLower);
                            mut_state.basis_matrix().update_pivot(i, best_d, tol_pivot_);
                        }
                    }
                }

                Basis std_b(m, n);
                for (Index i = 0; i < m; ++i) {
                    Index c_idx = mut_state.basis().get_basic_var(i);
                    if (c_idx < n) {
                        std_b.set_basic(c_idx, i);
                    } else {
                        // Find slack variable for constraint i to ensure linear independence
                        Index slack_col = -1;
                        for (Index j = 0; j < n; ++j) {
                            const auto& v = std_model.get_variable(j);
                            if (v.role != VariableRole::Structural && v.constraint_index == i) {
                                slack_col = j;
                                break;
                            }
                        }
                        if (slack_col != -1) {
                            std_b.set_basic(slack_col, i);
                        } else {
                            // Fallback: pick any unused column if row is structurally dependent
                            for (Index j = 0; j < n; ++j) {
                                if (std_b.get_status(j) != BasisStatus::Basic) {
                                    std_b.set_basic(j, i);
                                    break;
                                }
                            }
                        }
                    }
                }
                for (Index j = 0; j < n; ++j) {
                    if (std_b.get_status(j) != BasisStatus::Basic) {
                        BasisStatus st = mut_state.basis().get_status(j);
                        if (st == BasisStatus::Basic) st = BasisStatus::AtLower;
                        std_b.set_nonbasic(j, st);
                    }
                }
                std_b.rebuild_position_maps();
                res.basis = std_b;
            } else {
                res.basis = mut_state.basis();
            }
        } else {
            res.basis = mut_state.basis();
        }
        Float64 obj = orig_model.objective_offset();
        for (Index j = 0; j < orig_model.num_variables(); ++j) {
            obj += orig_model.get_variable(j).obj_coeff * res.primal_solution[j];
        }
        res.objective_value = obj;

        // Original Residual Verification using fast SpMV multiply
        const auto& orig_csc = orig_model.get_csc_matrix();
        std::vector<Float64> Ax(orig_model.num_constraints(), 0.0);
        orig_csc.multiply(res.primal_solution, Ax);

        Float64 max_row_res = 0.0;
        for (Index i = 0; i < orig_model.num_constraints(); ++i) {
            const auto& c = orig_model.get_constraint(i);
            Float64 row_val = Ax[i];
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
