#include "sovereign/barrier/barrier_solver.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <numeric>

namespace sovereign::barrier {

BarrierResult BarrierSolver::solve(const lp::LPModel& orig_model) const {
    BarrierResult res;

    // 1. Convert original LP model to standard form A_std * x_std = b_std, x_std >= 0
    lp::LPModel std_model = orig_model.convert_to_standard_form();
    Index m = std_model.num_constraints();
    Index n = std_model.num_variables();

    if (m == 0 || n == 0) {
        res.status = BarrierStatus::Optimal;
        res.iterations = 0;
        res.primal_solution.resize(orig_model.num_variables(), 0.0);
        res.objective_value = orig_model.objective_offset();
        res.is_primal_feasible = true;
        res.is_dual_feasible = true;
        return res;
    }

    const auto& A_csc = std_model.get_csc_matrix();

    // Gather variable lower bounds l_j and shift vector
    std::vector<Float64> l_shift(n, 0.0);
    for (Index j = 0; j < n; ++j) {
        Float64 lb = std_model.get_variable(j).lower_bound;
        if (lb > -INFINITY_VAL / 2.0) {
            l_shift[j] = lb;
        }
    }

    // Extract b (RHS) and shift b_shift = b - A * l_shift
    std::vector<Float64> b(m, 0.0);
    for (Index i = 0; i < m; ++i) {
        b[i] = std_model.get_constraint(i).rhs;
    }

    for (Index j = 0; j < n; ++j) {
        Float64 lj = l_shift[j];
        if (lj != 0.0) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                b[r_idx[k]] -= vals[k] * lj;
            }
        }
    }

    // Extract c (objective vector)
    std::vector<Float64> c(n, 0.0);
    bool is_min = (orig_model.objective_sense() == ObjectiveSense::Minimize);
    Float64 obj_l_shift = 0.0;
    for (Index j = 0; j < n; ++j) {
        Float64 c_j = is_min ? std_model.get_variable(j).obj_coeff : -std_model.get_variable(j).obj_coeff;
        c[j] = c_j;
        obj_l_shift += c_j * l_shift[j];
    }

    // 2. Infeasible Start Initialization (x > 0, s > 0, y = 0)
    Float64 b_norm2 = 0.0;
    for (Float64 val : b) b_norm2 += val * val;
    b_norm2 = std::sqrt(b_norm2);

    Float64 c_norm2 = 0.0;
    for (Float64 val : c) c_norm2 += val * val;
    c_norm2 = std::sqrt(c_norm2);

    Float64 init_x = std::max(1.0, b_norm2 / std::sqrt(static_cast<Float64>(n)));
    Float64 init_s = std::max(1.0, c_norm2 / std::sqrt(static_cast<Float64>(n)));

    std::vector<Float64> x(n, init_x);
    std::vector<Float64> s(n, init_s);
    std::vector<Float64> y(m, 0.0);

    SparseCholesky cholesky;
    cholesky.analyze_pattern(m);

    std::vector<Float64> r_p(m, 0.0);       // Primal residual b - A*x
    std::vector<Float64> r_d(n, 0.0);       // Dual residual c - A^T*y - s
    std::vector<Float64> d_sq(n, 0.0);      // D^2 = diag(x_j / s_j)
    std::vector<Float64> s_inv(n, 0.0);     // 1 / s_j
    std::vector<Float64> h_rhs(m, 0.0);     // Normal equations RHS
    std::vector<Float64> dy_aff(m, 0.0);    // Affine dual direction
    std::vector<Float64> ds_aff(n, 0.0);    // Affine dual slack direction
    std::vector<Float64> dx_aff(n, 0.0);    // Affine primal direction
    std::vector<Float64> dy_comb(m, 0.0);   // Combined dual direction
    std::vector<Float64> ds_comb(n, 0.0);   // Combined dual slack direction
    std::vector<Float64> dx_comb(n, 0.0);   // Combined primal direction

    Float64 b_norm_inf = 0.0;
    for (Float64 val : b) b_norm_inf = std::max(b_norm_inf, std::abs(val));

    Float64 c_norm_inf = 0.0;
    for (Float64 val : c) c_norm_inf = std::max(c_norm_inf, std::abs(val));

    Int64 iter = 0;
    bool converged = false;

    for (iter = 0; iter < config_.max_iterations; ++iter) {
        // Evaluate primal residual r_p = b - A*x
        for (Index i = 0; i < m; ++i) r_p[i] = b[i];
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                r_p[r_idx[k]] -= vals[k] * x[j];
            }
        }

        // Evaluate dual residual r_d = c - A^T*y - s
        for (Index j = 0; j < n; ++j) r_d[j] = c[j] - s[j];
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                Index r = r_idx[k];
                r_d[j] -= vals[k] * y[r];
            }
        }

        // Calculate duality gap & complementarity mu
        Float64 xs_sum = 0.0;
        for (Index j = 0; j < n; ++j) {
            xs_sum += x[j] * s[j];
        }
        Float64 mu = xs_sum / static_cast<Float64>(n);

        // Compute norm infinity of residuals
        Float64 norm_r_p = 0.0;
        for (Float64 val : r_p) norm_r_p = std::max(norm_r_p, std::abs(val));

        Float64 norm_r_d = 0.0;
        for (Float64 val : r_d) norm_r_d = std::max(norm_r_d, std::abs(val));

        Float64 primal_obj = 0.0;
        for (Index j = 0; j < n; ++j) primal_obj += c[j] * x[j];

        Float64 dual_obj = 0.0;
        for (Index i = 0; i < m; ++i) dual_obj += b[i] * y[i];

        Float64 rel_p_res = norm_r_p / (1.0 + b_norm_inf);
        Float64 rel_d_res = norm_r_d / (1.0 + c_norm_inf);
        Float64 rel_gap = std::abs(primal_obj - dual_obj) / (1.0 + std::abs(primal_obj));

        res.primal_residual = norm_r_p;
        res.dual_residual = norm_r_d;
        res.duality_gap = rel_gap;
        res.mu = mu;
        res.is_primal_feasible = (rel_p_res <= config_.tol_primal);
        res.is_dual_feasible = (rel_d_res <= config_.tol_dual);

        // Convergence Check
        if (rel_p_res <= config_.tol_primal &&
            rel_d_res <= config_.tol_dual &&
            rel_gap <= config_.tol_gap &&
            mu <= config_.tol_mu) {
            converged = true;
            break;
        }

        // Form D^2 = x_j / s_j (clamped to [1e-14, 1e14])
        for (Index j = 0; j < n; ++j) {
            Float64 val = x[j] / s[j];
            d_sq[j] = std::clamp(val, 1e-14, 1e14);
            s_inv[j] = 1.0 / s[j];
        }

        // Factorize M = A * D^2 * A^T + reg * I
        Status fact_status = cholesky.factorize(
            A_csc, d_sq, config_.min_pivot, config_.initial_regularization, config_.max_regularization
        );

        if (!is_ok(fact_status)) {
            res.status = BarrierStatus::NumericalFailure;
            res.message = "SparseCholesky factorization failed due to numerical instability";
            res.iterations = iter + 1;
            return res;
        }

        // --- STEP 1: Affine Predictor Step ---
        // h_aff = r_p + A * D^2 * r_d + A * x
        for (Index i = 0; i < m; ++i) h_rhs[i] = r_p[i];
        for (Index j = 0; j < n; ++j) {
            Float64 term = d_sq[j] * r_d[j] + x[j];
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                h_rhs[r_idx[k]] += vals[k] * term;
            }
        }

        if (!is_ok(cholesky.solve(h_rhs, dy_aff))) {
            res.status = BarrierStatus::NumericalFailure;
            res.message = "SparseCholesky solve failed on affine predictor step";
            res.iterations = iter + 1;
            return res;
        }

        // ds_aff = r_d - A^T * dy_aff
        for (Index j = 0; j < n; ++j) ds_aff[j] = r_d[j];
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                ds_aff[j] -= vals[k] * dy_aff[r_idx[k]];
            }
        }

        // dx_aff = -x - D^2 * ds_aff
        for (Index j = 0; j < n; ++j) {
            dx_aff[j] = -x[j] - d_sq[j] * ds_aff[j];
        }

        // Calculate affine max step lengths
        Float64 alpha_p_aff = 1.0;
        Float64 alpha_d_aff = 1.0;
        for (Index j = 0; j < n; ++j) {
            if (dx_aff[j] < 0.0) {
                alpha_p_aff = std::min(alpha_p_aff, -x[j] / dx_aff[j]);
            }
            if (ds_aff[j] < 0.0) {
                alpha_d_aff = std::min(alpha_d_aff, -s[j] / ds_aff[j]);
            }
        }

        // Calculate mu_aff & centering parameter sigma = (mu_aff / mu)^3
        Float64 mu_aff_sum = 0.0;
        for (Index j = 0; j < n; ++j) {
            Float64 x_next = x[j] + alpha_p_aff * dx_aff[j];
            Float64 s_next = s[j] + alpha_d_aff * ds_aff[j];
            mu_aff_sum += x_next * s_next;
        }
        Float64 mu_aff = mu_aff_sum / static_cast<Float64>(n);
        Float64 sigma = std::pow(mu_aff / mu, 3);
        sigma = std::clamp(sigma, 0.0, 1.0);

        // --- STEP 2: Combined Corrector Step ---
        // r_xs = sigma*mu - x_j*s_j - dx_aff_j*ds_aff_j
        // h_comb = r_p + A * D^2 * r_d - A * S^-1 * r_xs
        for (Index i = 0; i < m; ++i) h_rhs[i] = r_p[i];
        for (Index j = 0; j < n; ++j) {
            Float64 r_xs_j = sigma * mu - x[j] * s[j] - dx_aff[j] * ds_aff[j];
            Float64 term = d_sq[j] * r_d[j] - s_inv[j] * r_xs_j;
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                h_rhs[r_idx[k]] += vals[k] * term;
            }
        }

        if (!is_ok(cholesky.solve(h_rhs, dy_comb))) {
            res.status = BarrierStatus::NumericalFailure;
            res.message = "SparseCholesky solve failed on corrector step";
            res.iterations = iter + 1;
            return res;
        }

        // ds_comb = r_d - A^T * dy_comb
        for (Index j = 0; j < n; ++j) ds_comb[j] = r_d[j];
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                ds_comb[j] -= vals[k] * dy_comb[r_idx[k]];
            }
        }

        // dx_comb = S^-1 * (r_xs - X * ds_comb)
        for (Index j = 0; j < n; ++j) {
            Float64 r_xs_j = sigma * mu - x[j] * s[j] - dx_aff[j] * ds_aff[j];
            dx_comb[j] = s_inv[j] * (r_xs_j - x[j] * ds_comb[j]);
        }

        // --- STEP 3: Step Sizes & Iterates Update ---
        Float64 alpha_p_max = 1.0;
        Float64 alpha_d_max = 1.0;
        for (Index j = 0; j < n; ++j) {
            if (dx_comb[j] < 0.0) {
                alpha_p_max = std::min(alpha_p_max, -x[j] / dx_comb[j]);
            }
            if (ds_comb[j] < 0.0) {
                alpha_d_max = std::min(alpha_d_max, -s[j] / ds_comb[j]);
            }
        }

        Float64 tau = config_.step_safety_factor; // 0.995
        Float64 alpha_p = std::min(1.0, tau * alpha_p_max);
        Float64 alpha_d = std::min(1.0, tau * alpha_d_max);

        for (Index j = 0; j < n; ++j) {
            x[j] += alpha_p * dx_comb[j];
            s[j] += alpha_d * ds_comb[j];
        }
        for (Index i = 0; i < m; ++i) {
            y[i] += alpha_d * dy_comb[i];
        }
    }

    res.iterations = converged ? iter : config_.max_iterations;
    if (converged) {
        res.status = BarrierStatus::Optimal;
        res.message = "Barrier solver converged to optimal solution";
    } else {
        res.status = BarrierStatus::IterationLimit;
        res.message = "Barrier solver reached maximum iteration limit";
    }

    // Recover original primal variables: x_unshift = x + l_shift
    std::vector<Float64> x_unshift(n, 0.0);
    for (Index j = 0; j < n; ++j) {
        x_unshift[j] = x[j] + l_shift[j];
    }

    res.primal_solution = std_model.standard_form_map().recover_original_solution(x_unshift);
    res.dual_solution = y;
    res.dual_slacks = s;

    // Calculate final original objective value
    Float64 orig_obj = orig_model.objective_offset();
    for (Index j = 0; j < orig_model.num_variables(); ++j) {
        orig_obj += orig_model.get_variable(j).obj_coeff * res.primal_solution[j];
    }
    res.objective_value = orig_obj;

    return res;
}

} // namespace sovereign::barrier
