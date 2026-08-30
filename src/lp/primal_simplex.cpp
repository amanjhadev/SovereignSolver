#include "sovereign/lp/primal_simplex.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <chrono>

namespace sovereign::lp {

PrimalSimplexSolver::PrimalSimplexSolver(const SolverConfig& config)
    : config_(config),
      tol_primal_(config.feasibility_tolerance),
      tol_dual_(config.optimality_tolerance),
      tol_pivot_(config.pivot_tolerance),
      max_iterations_(config.max_iterations) {}

SimplexStatus PrimalSimplexSolver::solve(SimplexState& state, Int64& total_iters) {
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

        // 1. Select Entering Variable (most negative reduced cost direction)
        const auto& rc = state.reduced_costs();
        Float64 min_rc = 0.0;
        Index entering_col = -1;

        for (Index j = 0; j < state.num_cols(); ++j) {
            BasisStatus st = state.basis().get_status(j);
            if (st == BasisStatus::Basic || st == BasisStatus::Fixed) continue;

            Float64 c_bar = rc[j];
            if (st == BasisStatus::AtLower && c_bar < min_rc - tol_dual_) {
                min_rc = c_bar;
                entering_col = j;
            } else if (st == BasisStatus::AtUpper && -c_bar < min_rc - tol_dual_) {
                min_rc = -c_bar;
                entering_col = j;
            }
        }

        // If all reduced costs optimal, return Optimal!
        if (entering_col == -1) {
            return SimplexStatus::Optimal;
        }

        // 2. FTRAN: compute direction d = B^-1 A_q
        std::vector<Float64> d(state.num_rows(), 0.0);
        if (!is_ok(state.compute_ftran_direction(entering_col, d))) {
            return SimplexStatus::NumericalFailure;
        }

        BasisStatus ent_st = state.basis().get_status(entering_col);
        const auto& ent_var = state.std_model().get_variable(entering_col);

        // 3. Bounded Primal Ratio Test
        Float64 min_step = INFINITY_VAL;
        Index leaving_slot = -1;
        BasisStatus leaving_new_status = BasisStatus::AtLower;
        bool is_bound_flip = false;

        // Check bound flip for entering variable itself
        if (ent_var.lower_bound > -INFINITY_VAL && ent_var.upper_bound < INFINITY_VAL) {
            min_step = ent_var.upper_bound - ent_var.lower_bound;
            is_bound_flip = true;
        }

        for (Index i = 0; i < state.num_rows(); ++i) {
            Float64 d_i = d[i];
            Index col = state.basis().get_basic_var(i);
            const auto& var = state.std_model().get_variable(col);
            Float64 x_bi = state.x_B()[i];

            if (ent_st == BasisStatus::AtLower) {
                // Entering variable increases from lower bound
                if (d_i > tol_pivot_ && var.lower_bound > -INFINITY_VAL) {
                    Float64 ratio = (x_bi - var.lower_bound) / d_i;
                    if (ratio < min_step) {
                        min_step = ratio;
                        leaving_slot = i;
                        leaving_new_status = BasisStatus::AtLower;
                        is_bound_flip = false;
                    }
                } else if (d_i < -tol_pivot_ && var.upper_bound < INFINITY_VAL) {
                    Float64 ratio = (var.upper_bound - x_bi) / (-d_i);
                    if (ratio < min_step) {
                        min_step = ratio;
                        leaving_slot = i;
                        leaving_new_status = BasisStatus::AtUpper;
                        is_bound_flip = false;
                    }
                }
            } else if (ent_st == BasisStatus::AtUpper) {
                // Entering variable decreases from upper bound
                if (d_i < -tol_pivot_ && var.lower_bound > -INFINITY_VAL) {
                    Float64 ratio = (x_bi - var.lower_bound) / (-d_i);
                    if (ratio < min_step) {
                        min_step = ratio;
                        leaving_slot = i;
                        leaving_new_status = BasisStatus::AtLower;
                        is_bound_flip = false;
                    }
                } else if (d_i > tol_pivot_ && var.upper_bound < INFINITY_VAL) {
                    Float64 ratio = (var.upper_bound - x_bi) / d_i;
                    if (ratio < min_step) {
                        min_step = ratio;
                        leaving_slot = i;
                        leaving_new_status = BasisStatus::AtUpper;
                        is_bound_flip = false;
                    }
                }
            }
        }

        if (min_step >= INFINITY_VAL / 2.0) {
            return SimplexStatus::Unbounded;
        }

        if (is_bound_flip) {
            // Flip entering variable bound without basis pivot
            BasisStatus flip_st = (ent_st == BasisStatus::AtLower) ? BasisStatus::AtUpper : BasisStatus::AtLower;
            state.basis().set_nonbasic(entering_col, flip_st);
        } else {
            // Execute basis pivot & apply Product-Form Eta update
            state.basis().pivot(entering_col, leaving_slot, leaving_new_status);
            Status up_st = state.basis_matrix().update_pivot(leaving_slot, d, tol_pivot_);
            if (!is_ok(up_st)) {
                // Instability detected -> trigger full refactorization
                if (!is_ok(state.factorize_basis(config_))) {
                    return SimplexStatus::NumericalFailure;
                }
            }
        }

        total_iters++;
        state.increment_iterations();
    }

    return SimplexStatus::IterationLimit;
}

} // namespace sovereign::lp
