#include "sovereign/qp/qp_solver.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <numeric>

namespace sovereign::qp {

QPResult QPSolver::solve(const QPModel& model) const {
    QPResult res;

    // 1. Validate Convexity (Q >= 0)
    QPStatus conv_stat = model.validate_convexity();
    if (conv_stat == QPStatus::NonConvexModel) {
        res.status = QPStatus::NonConvexModel;
        res.message = "QPModel validation failed: Q matrix is indefinite (non-convex QP)";
        return res;
    }

    Index orig_n = model.num_variables();
    if (orig_n == 0) {
        res.status = QPStatus::Optimal;
        res.iterations = 0;
        res.objective_value = model.objective_offset();
        res.is_primal_feasible = true;
        res.is_dual_feasible = true;
        return res;
    }

    // 2. Convert underlying LP model to standard form A_std * x_std = b_std, x_std >= 0
    lp::LPModel std_model = model.lp_model().convert_to_standard_form();
    Index m = std_model.num_constraints();
    Index n = std_model.num_variables();

    if (m == 0) {
        // Handle unconstrained / bound-only QP
        res.primal_solution.resize(orig_n, 0.0);
        for (Index j = 0; j < orig_n; ++j) {
            const auto& var = model.lp_model().get_variable(j);
            Float64 lb = var.lower_bound;
            Float64 ub = var.upper_bound;
            Float64 c_j = var.obj_coeff;

            Float64 q_jj = 0.0;
            for (const auto& entry : model.q_entries()) {
                if (entry.i == j && entry.j == j) {
                    q_jj += entry.val;
                }
            }

            Float64 opt_x = 0.0;
            if (q_jj > 1e-12) {
                opt_x = -c_j / q_jj;
            } else {
                opt_x = (c_j >= 0.0) ? lb : ub;
            }

            if (lb > -INFINITY_VAL / 2.0) opt_x = std::max(opt_x, lb);
            if (ub < INFINITY_VAL / 2.0) opt_x = std::min(opt_x, ub);

            res.primal_solution[j] = opt_x;
        }

        res.status = QPStatus::Optimal;
        res.iterations = 0;
        res.objective_value = model.evaluate_objective(res.primal_solution);
        res.is_primal_feasible = true;
        res.is_dual_feasible = true;
        return res;
    }

    const auto& A_csc = std_model.get_csc_matrix();

    // Map Q matrix entries to standard form variables
    std::vector<Float64> Q_std_dense(static_cast<std::size_t>(n) * n, 0.0);

    for (const auto& entry : model.q_entries()) {
        Index orig_i = entry.i;
        Index orig_j = entry.j;

        Index std_i = -1;
        Index std_j = -1;

        for (Index s = 0; s < n; ++s) {
            const auto& std_v = std_model.get_variable(s);
            if (std_v.original_var_index == orig_i || s == orig_i || (orig_i < model.num_variables() && std_v.name == model.lp_model().get_variable(orig_i).name)) std_i = s;
            if (std_v.original_var_index == orig_j || s == orig_j || (orig_j < model.num_variables() && std_v.name == model.lp_model().get_variable(orig_j).name)) std_j = s;
        }

        if (std_i >= 0 && std_j >= 0) {
            Q_std_dense[std_i * n + std_j] += entry.val;
            if (std_i != std_j) {
                Q_std_dense[std_j * n + std_i] += entry.val;
            }
        }
    }

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

    // Extract c (linear objective) and shift c_shift = c + Q * l_shift
    std::vector<Float64> c(n, 0.0);
    bool is_min = (model.objective_sense() == ObjectiveSense::Minimize);

    for (Index j = 0; j < n; ++j) {
        c[j] = is_min ? std_model.get_variable(j).obj_coeff : -std_model.get_variable(j).obj_coeff;
    }

    // Add Q * l_shift to c
    for (Index i = 0; i < n; ++i) {
        for (Index j = 0; j < n; ++j) {
            c[i] += Q_std_dense[i * n + j] * l_shift[j];
        }
    }

    // 3. Positive Initialization (x > 0, s > 0, y = 0)
    Float64 b_norm2 = 0.0;
    for (Float64 val : b) b_norm2 += val * val;
    b_norm2 = std::sqrt(b_norm2);

    Float64 c_norm2 = 0.0;
    for (Float64 val : c) c_norm2 += val * val;
    c_norm2 = std::sqrt(c_norm2);

    Float64 init_x = std::max(1.0, b_norm2 / std::sqrt(static_cast<Float64>(n)));
    Float64 init_s = std::max(1.0, c_norm2 / std::sqrt(static_cast<Float64>(n)));

    std::vector<Float64> x(n);
    for (Index j = 0; j < n; ++j) {
        Float64 ub = std_model.get_variable(j).upper_bound;
        if (ub < INFINITY_VAL / 2.0 && ub > 0.0) {
            x[j] = std::min(init_x, 0.5 * ub);
        } else {
            x[j] = init_x;
        }
    }
    std::vector<Float64> s(n, init_s);
    std::vector<Float64> y(m, 0.0);

    barrier::SparseCholesky cholesky;
    cholesky.analyze_pattern(m);

    std::vector<Float64> r_p(m, 0.0);
    std::vector<Float64> r_d(n, 0.0);
    std::vector<Float64> Qx(n, 0.0);
    std::vector<Float64> H_dense(static_cast<std::size_t>(n) * n, 0.0);
    std::vector<Float64> H_inv(static_cast<std::size_t>(n) * n, 0.0);
    std::vector<Float64> h_rhs(m, 0.0);
    std::vector<Float64> rhs_x(n, 0.0);
    std::vector<Float64> dy_aff(m, 0.0);
    std::vector<Float64> ds_aff(n, 0.0);
    std::vector<Float64> dx_aff(n, 0.0);
    std::vector<Float64> dy_comb(m, 0.0);
    std::vector<Float64> ds_comb(n, 0.0);
    std::vector<Float64> dx_comb(n, 0.0);

    Float64 b_norm_inf = 0.0;
    for (Float64 val : b) b_norm_inf = std::max(b_norm_inf, std::abs(val));

    Float64 c_norm_inf = 0.0;
    for (Float64 val : c) c_norm_inf = std::max(c_norm_inf, std::abs(val));

    Int64 iter = 0;
    bool converged = false;

    for (iter = 0; iter < config_.max_iterations; ++iter) {
        // Compute Q * x
        std::fill(Qx.begin(), Qx.end(), 0.0);
        for (Index i = 0; i < n; ++i) {
            for (Index j = 0; j < n; ++j) {
                Qx[i] += Q_std_dense[i * n + j] * x[j];
            }
        }

        // Evaluate primal residual r_p = b - A*x
        for (Index i = 0; i < m; ++i) r_p[i] = b[i];
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                r_p[r_idx[k]] -= vals[k] * x[j];
            }
        }

        // Evaluate dual residual r_d = c + Q*x - A^T*y - s
        for (Index j = 0; j < n; ++j) r_d[j] = c[j] + Qx[j] - s[j];
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

        Float64 quad_term = 0.0;
        for (Index j = 0; j < n; ++j) quad_term += 0.5 * Qx[j] * x[j];

        Float64 primal_obj = quad_term;
        for (Index j = 0; j < n; ++j) primal_obj += c[j] * x[j];

        Float64 dual_obj = -quad_term;
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

        // Form H = Q + X^-1 * S  (H_jj = Q_jj + s_j / x_j)
        std::copy(Q_std_dense.begin(), Q_std_dense.end(), H_dense.begin());
        for (Index j = 0; j < n; ++j) {
            H_dense[j * n + j] += s[j] / x[j];
        }

        // Cholesky L * L^T Factorization of H (H is strictly positive definite H > 0)
        std::vector<Float64> L_h(static_cast<std::size_t>(n) * n, 0.0);
        bool h_pos_def = true;

        for (Index i = 0; i < n; ++i) {
            Float64 sum = H_dense[i * n + i];
            for (Index k = 0; k < i; ++k) {
                Float64 lik = L_h[i * n + k];
                sum -= lik * lik;
            }
            if (sum <= 1e-14) {
                h_pos_def = false;
                break;
            }
            Float64 l_ii = std::sqrt(sum);
            L_h[i * n + i] = l_ii;

            for (Index j = i + 1; j < n; ++j) {
                Float64 s_val = H_dense[j * n + i];
                for (Index k = 0; k < i; ++k) {
                    s_val -= L_h[j * n + k] * L_h[i * n + k];
                }
                L_h[j * n + i] = s_val / l_ii;
            }
        }

        if (!h_pos_def) {
            res.status = QPStatus::NumericalFailure;
            res.message = "Cholesky factorization of H matrix failed (not positive definite)";
            res.iterations = iter + 1;
            return res;
        }

        // Compute H_inv = L^-T * L^-1 via forward & back substitution for each unit e_j
        std::fill(H_inv.begin(), H_inv.end(), 0.0);
        std::vector<Float64> y_col(n, 0.0);
        std::vector<Float64> x_col(n, 0.0);

        for (Index j = 0; j < n; ++j) {
            std::fill(y_col.begin(), y_col.end(), 0.0);
            for (Index i = 0; i < n; ++i) {
                Float64 rhs = (i == j) ? 1.0 : 0.0;
                for (Index k = 0; k < i; ++k) {
                    rhs -= L_h[i * n + k] * y_col[k];
                }
                y_col[i] = rhs / L_h[i * n + i];
            }

            std::fill(x_col.begin(), x_col.end(), 0.0);
            for (Index i = n; i > 0; --i) {
                Index idx = i - 1;
                Float64 rhs = y_col[idx];
                for (Index k = idx + 1; k < n; ++k) {
                    rhs -= L_h[k * n + idx] * x_col[k];
                }
                x_col[idx] = rhs / L_h[idx * n + idx];
            }

            for (Index i = 0; i < n; ++i) {
                H_inv[i * n + j] = x_col[i];
            }
        }

        // Form d_weights array (diagonal of H_inv) for SparseCholesky factorize interface
        std::vector<Float64> d_weights(n, 0.0);
        for (Index j = 0; j < n; ++j) {
            d_weights[j] = H_inv[j * n + j];
        }

        // Factorize M = A * H_inv * A^T
        Status fact_status = cholesky.factorize(
            A_csc, d_weights, config_.min_pivot, config_.initial_regularization, config_.max_regularization
        );

        if (!is_ok(fact_status)) {
            res.status = QPStatus::NumericalFailure;
            res.message = "SparseCholesky factorization failed on QP normal equations system";
            res.iterations = iter + 1;
            return res;
        }

        // --- STEP 1: Affine Predictor Step ---
        // r_xs_aff = -X * S * e  =>  X^-1 * r_xs_aff = -s
        // rhs_x = r_d - X^-1 * r_xs_aff = r_d + s
        for (Index j = 0; j < n; ++j) rhs_x[j] = r_d[j] + s[j];
        
        // Hinv_rhs_x = H_inv * rhs_x
        std::vector<Float64> Hinv_rhs_x(n, 0.0);
        for (Index i = 0; i < n; ++i) {
            for (Index j = 0; j < n; ++j) {
                Hinv_rhs_x[i] += H_inv[i * n + j] * rhs_x[j];
            }
        }

        // h_aff = r_p + A * Hinv_rhs_x
        for (Index i = 0; i < m; ++i) h_rhs[i] = r_p[i];
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                h_rhs[r_idx[k]] += vals[k] * Hinv_rhs_x[j];
            }
        }

        if (!is_ok(cholesky.solve(h_rhs, dy_aff))) {
            res.status = QPStatus::NumericalFailure;
            res.message = "SparseCholesky solve failed on affine predictor step";
            res.iterations = iter + 1;
            return res;
        }

        // dx_aff = H_inv * (A^T * dy_aff - rhs_x)
        std::vector<Float64> At_dy(n, 0.0);
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                At_dy[j] += vals[k] * dy_aff[r_idx[k]];
            }
        }

        for (Index i = 0; i < n; ++i) {
            Float64 diff = At_dy[i] - rhs_x[i];
            dx_aff[i] = 0.0;
            for (Index j = 0; j < n; ++j) {
                dx_aff[i] += H_inv[i * n + j] * diff;
            }
        }

        // ds_aff = -s - (s / x) * dx_aff
        for (Index j = 0; j < n; ++j) {
            ds_aff[j] = -s[j] - (s[j] / x[j]) * dx_aff[j];
        }

        // Calculate affine max step lengths
        Float64 alpha_p_aff = 1.0;
        Float64 alpha_d_aff = 1.0;
        for (Index j = 0; j < n; ++j) {
            if (dx_aff[j] < 0.0) alpha_p_aff = std::min(alpha_p_aff, -x[j] / dx_aff[j]);
            if (ds_aff[j] < 0.0) alpha_d_aff = std::min(alpha_d_aff, -s[j] / ds_aff[j]);
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
        // gamma_j = sigma*mu - dx_aff_j*ds_aff_j
        // rhs_x[j] = r_d[j] + s[j] - gamma_j / x[j]
        for (Index j = 0; j < n; ++j) {
            Float64 gamma_j = sigma * mu - dx_aff[j] * ds_aff[j];
            rhs_x[j] = r_d[j] + s[j] - gamma_j / x[j];
        }

        std::fill(Hinv_rhs_x.begin(), Hinv_rhs_x.end(), 0.0);
        for (Index i = 0; i < n; ++i) {
            for (Index j = 0; j < n; ++j) {
                Hinv_rhs_x[i] += H_inv[i * n + j] * rhs_x[j];
            }
        }

        for (Index i = 0; i < m; ++i) h_rhs[i] = r_p[i];
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                h_rhs[r_idx[k]] += vals[k] * Hinv_rhs_x[j];
            }
        }

        if (!is_ok(cholesky.solve(h_rhs, dy_comb))) {
            res.status = QPStatus::NumericalFailure;
            res.message = "SparseCholesky solve failed on combined corrector step";
            res.iterations = iter + 1;
            return res;
        }

        std::fill(At_dy.begin(), At_dy.end(), 0.0);
        for (Index j = 0; j < n; ++j) {
            auto r_idx = A_csc.get_col_row_indices(j);
            auto vals = A_csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                At_dy[j] += vals[k] * dy_comb[r_idx[k]];
            }
        }

        for (Index i = 0; i < n; ++i) {
            Float64 diff = At_dy[i] - rhs_x[i];
            dx_comb[i] = 0.0;
            for (Index j = 0; j < n; ++j) {
                dx_comb[i] += H_inv[i * n + j] * diff;
            }
        }

        for (Index j = 0; j < n; ++j) {
            Float64 gamma_j = sigma * mu - dx_aff[j] * ds_aff[j];
            ds_comb[j] = (gamma_j - x[j] * s[j] - s[j] * dx_comb[j]) / x[j];
        }

        // --- STEP 3: Step Sizes & Iterates Update ---
        Float64 alpha_p_max = 1.0;
        Float64 alpha_d_max = 1.0;
        for (Index j = 0; j < n; ++j) {
            if (dx_comb[j] < 0.0) alpha_p_max = std::min(alpha_p_max, -x[j] / dx_comb[j]);
            if (ds_comb[j] < 0.0) alpha_d_max = std::min(alpha_d_max, -s[j] / ds_comb[j]);
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
        res.status = QPStatus::Optimal;
        res.message = "QP solver converged to optimal solution";
    } else {
        res.status = QPStatus::IterationLimit;
        res.message = "QP solver reached maximum iteration limit";
    }

    // Recover original primal variables: x_unshift = x + l_shift
    std::vector<Float64> x_unshift(n, 0.0);
    for (Index j = 0; j < n; ++j) {
        x_unshift[j] = x[j] + l_shift[j];
    }

    std::vector<Float64> orig_sol(model.num_variables(), 0.0);
    for (Index s = 0; s < n; ++s) {
        Index orig_idx = std_model.get_variable(s).original_var_index;
        if (orig_idx < 0 || orig_idx >= model.num_variables()) {
            for (Index k = 0; k < model.num_variables(); ++k) {
                if (model.lp_model().get_variable(k).name == std_model.get_variable(s).name) {
                    orig_idx = k;
                    break;
                }
            }
        }
        if (orig_idx < 0 || orig_idx >= model.num_variables()) {
            orig_idx = s;
        }
        if (orig_idx >= 0 && orig_idx < model.num_variables()) {
            orig_sol[orig_idx] = x_unshift[s];
        }
    }

    res.primal_solution = std::move(orig_sol);
    res.dual_solution = y;
    res.dual_slacks = s;

    // Calculate final original objective value
    res.objective_value = model.evaluate_objective(res.primal_solution);

    return res;
}

} // namespace sovereign::qp
