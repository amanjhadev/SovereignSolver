#include "sovereign/lp/simplex_state.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace sovereign::lp {

SimplexState::SimplexState(const LPModel& std_model, Basis basis)
    : std_model_(&std_model), basis_(std::move(basis)) {
    m_ = std_model_->num_constraints();
    n_ = std_model_->num_variables();

    if (basis_.num_rows() != m_ || basis_.num_cols() != n_) {
        throw std::invalid_argument("Basis dimension mismatch with standard form model");
    }

    x_B_.assign(m_, 0.0);
    x_all_.assign(n_, 0.0);
    y_.assign(m_, 0.0);
    reduced_costs_.assign(n_, 0.0);
    b_eff_.assign(m_, 0.0);
}

Status SimplexState::factorize_basis(const SolverConfig& config) {
    if (!basis_.is_valid()) {
        is_factored_ = false;
        return Status::InvalidInput;
    }
    const auto& csc = std_model_->get_csc_matrix();
    Status status = basis_matrix_.factorize(csc, basis_, config);
    is_factored_ = is_ok(status);
    return status;
}

void SimplexState::update_nonbasic_variable_values() {
    for (Index j = 0; j < n_; ++j) {
        if (basis_.get_status(j) != BasisStatus::Basic) {
            const auto& var = std_model_->get_variable(j);
            BasisStatus st = basis_.get_status(j);
            if (st == BasisStatus::AtLower || st == BasisStatus::Fixed) {
                x_all_[j] = var.lower_bound;
            } else if (st == BasisStatus::AtUpper) {
                x_all_[j] = var.upper_bound;
            } else { // Free
                x_all_[j] = 0.0;
            }
        }
    }
}

Status SimplexState::compute_b_eff() {
    for (Index i = 0; i < m_; ++i) {
        b_eff_[i] = std_model_->get_constraint(i).rhs;
    }

    update_nonbasic_variable_values();

    const auto& csc = std_model_->get_csc_matrix();
    for (Index j = 0; j < n_; ++j) {
        if (basis_.get_status(j) != BasisStatus::Basic) {
            Float64 xj = x_all_[j];
            if (xj == 0.0) continue;

            auto r_indices = csc.get_col_row_indices(j);
            auto c_vals = csc.get_col_values(j);
            for (std::size_t k = 0; k < r_indices.size(); ++k) {
                b_eff_[r_indices[k]] -= c_vals[k] * xj;
            }
        }
    }

    return Status::Ok;
}

Status SimplexState::solve_primal_solution() {
    if (!is_factored_) {
        return Status::Error;
    }

    Status status = compute_b_eff();
    if (!is_ok(status)) return status;

    status = basis_matrix_.solve_primal(b_eff_, x_B_);
    if (!is_ok(status)) return status;

    for (Index i = 0; i < m_; ++i) {
        Index col = basis_.get_basic_var(i);
        x_all_[col] = x_B_[i];
    }

    return Status::Ok;
}

Status SimplexState::solve_dual_solution() {
    if (!is_factored_) {
        return Status::Error;
    }

    std::vector<Float64> c_B(m_, 0.0);
    for (Index i = 0; i < m_; ++i) {
        Index col = basis_.get_basic_var(i);
        c_B[i] = std_model_->get_variable(col).obj_coeff;
    }

    Status status = basis_matrix_.solve_dual(c_B, y_);
    if (is_ok(status)) {
        compute_reduced_costs();
    }
    return status;
}

void SimplexState::compute_reduced_costs() {
    const auto& csc = std_model_->get_csc_matrix();
    for (Index j = 0; j < n_; ++j) {
        Float64 c_j = std_model_->get_variable(j).obj_coeff;
        auto r_indices = csc.get_col_row_indices(j);
        auto c_vals = csc.get_col_values(j);
        Float64 yA = 0.0;
        for (std::size_t k = 0; k < r_indices.size(); ++k) {
            yA += y_[r_indices[k]] * c_vals[k];
        }
        reduced_costs_[j] = c_j - yA;
    }
}

Status SimplexState::compute_ftran_direction(Index entering_col, std::vector<Float64>& d_out) const {
    if (!is_factored_) {
        return Status::Error;
    }
    std::vector<Float64> A_q(m_, 0.0);
    const auto& csc = std_model_->get_csc_matrix();
    auto r_indices = csc.get_col_row_indices(entering_col);
    auto c_vals = csc.get_col_values(entering_col);
    for (std::size_t k = 0; k < r_indices.size(); ++k) {
        A_q[r_indices[k]] = c_vals[k];
    }
    return basis_matrix_.solve_primal(A_q, d_out);
}

Status SimplexState::compute_btran_row(Index leaving_slot, std::vector<Float64>& y_p_out) const {
    if (!is_factored_) {
        return Status::Error;
    }
    if (leaving_slot < 0 || leaving_slot >= m_) {
        return Status::DimensionMismatch;
    }
    std::vector<Float64> e_p(m_, 0.0);
    e_p[leaving_slot] = 1.0;
    return basis_matrix_.solve_dual(e_p, y_p_out);
}

void SimplexState::compute_row_alphas(std::span<const Float64> y_p, std::vector<Float64>& alphas_out) const {
    alphas_out.assign(n_, 0.0);
    const auto& csc = std_model_->get_csc_matrix();
    for (Index j = 0; j < n_; ++j) {
        auto r_indices = csc.get_col_row_indices(j);
        auto c_vals = csc.get_col_values(j);
        Float64 val = 0.0;
        for (std::size_t k = 0; k < r_indices.size(); ++k) {
            val += y_p[r_indices[k]] * c_vals[k];
        }
        alphas_out[j] = val;
    }
}

bool SimplexState::check_primal_feasibility(Float64 tolerance, Float64& max_violation, Index& worst_row) const {
    max_violation = 0.0;
    worst_row = -1;

    for (Index i = 0; i < m_; ++i) {
        Index col = basis_.get_basic_var(i);
        const auto& var = std_model_->get_variable(col);
        Float64 val = x_B_[i];

        Float64 viol_lb = var.lower_bound - val;
        Float64 viol_ub = val - var.upper_bound;

        Float64 max_row_viol = std::max(0.0, std::max(viol_lb, viol_ub));
        if (max_row_viol > max_violation) {
            max_violation = max_row_viol;
            worst_row = i;
        }
    }

    return max_violation <= tolerance;
}

bool SimplexState::check_dual_feasibility(Float64 tolerance, Float64& max_violation, Index& worst_col) const {
    max_violation = 0.0;
    worst_col = -1;

    for (Index j = 0; j < n_; ++j) {
        BasisStatus st = basis_.get_status(j);
        if (st == BasisStatus::Basic || st == BasisStatus::Fixed) continue;

        Float64 dj = reduced_costs_[j];
        Float64 viol = 0.0;

        if (st == BasisStatus::AtLower) {
            if (dj < -tolerance) viol = -dj;
        } else if (st == BasisStatus::AtUpper) {
            if (dj > tolerance) viol = dj;
        } else if (st == BasisStatus::Free) {
            viol = std::abs(dj);
        }

        if (viol > max_violation) {
            max_violation = viol;
            worst_col = j;
        }
    }

    return max_violation <= tolerance;
}

Float64 SimplexState::compute_objective_value() const {
    Float64 obj = std_model_->objective_offset();
    for (Index j = 0; j < n_; ++j) {
        obj += std_model_->get_variable(j).obj_coeff * x_all_[j];
    }
    return obj;
}

} // namespace sovereign::lp
