#include "sovereign/lp/basis_matrix.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include <stdexcept>
#include <algorithm>

namespace sovereign::lp {

Status BasisMatrix::factorize(const linalg::CSCMatrix& A, const Basis& basis, const SolverConfig& config) {
    if (!basis.is_valid()) {
        is_factored_ = false;
        return Status::InvalidInput;
    }

    m_ = A.num_rows();
    if (m_ != basis.num_rows()) {
        is_factored_ = false;
        return Status::DimensionMismatch;
    }

    updates_.clear(); // Clear any existing Eta updates on full refactorization
    max_updates_ = config.max_refactorization_interval;

    // Build m x m CSC matrix B from basic columns of A
    linalg::COOMatrix B_coo(m_, m_);

    for (Index j = 0; j < m_; ++j) {
        Index orig_col = basis.get_basic_var(j);
        auto r_indices = A.get_col_row_indices(orig_col);
        auto c_values  = A.get_col_values(orig_col);

        for (std::size_t k = 0; k < r_indices.size(); ++k) {
            B_coo.add_entry(r_indices[k], j, c_values[k]);
        }
    }

    B_csc_ = B_coo.to_csc();

    Status status = lu_.factorize(B_csc_, config);
    if (is_ok(status)) {
        is_factored_ = true;
    } else {
        is_factored_ = false;
    }

    return status;
}

Status BasisMatrix::update_pivot(Index slot_p, std::span<const Float64> ftran_d, Float64 pivot_tol) {
    if (!is_factored_) return Status::Error;
    if (slot_p < 0 || slot_p >= m_) return Status::InvalidInput;

    Float64 alpha_p = ftran_d[slot_p];
    if (std::abs(alpha_p) < pivot_tol) {
        return Status::NumericalInstability; // Unstable pivot magnitude -> force refactorization
    }

    EtaUpdate eta;
    eta.slot_p = slot_p;
    eta.alpha_p = alpha_p;

    for (Index i = 0; i < m_; ++i) {
        if (i != slot_p && std::abs(ftran_d[i]) > 1e-14) {
            eta.nz_rows.push_back(i);
            eta.nz_vals.push_back(ftran_d[i]);
        }
    }

    updates_.push_back(std::move(eta));
    return Status::Ok;
}

Status BasisMatrix::solve_primal(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) {
        return Status::Error;
    }
    if (updates_.empty()) {
        return lu_.solve(b, x);
    }

    // 1. Solve base system B^(0) y = b
    std::vector<Float64> y(m_, 0.0);
    Status st = lu_.solve(b, y);
    if (!is_ok(st)) return st;

    // 2. Apply Eta inverses in forward order: y <- E_1^-1 y, ..., y <- E_k^-1 y
    for (const auto& eta : updates_) {
        Index p = eta.slot_p;
        Float64 w = y[p] / eta.alpha_p;
        y[p] = w;
        for (std::size_t k = 0; k < eta.nz_rows.size(); ++k) {
            Index r = eta.nz_rows[k];
            y[r] -= eta.nz_vals[k] * w;
        }
    }

    std::copy(y.begin(), y.end(), x.begin());
    return Status::Ok;
}

Status BasisMatrix::solve_dual(std::span<const Float64> c_B, std::span<Float64> y) const {
    if (!is_factored_) {
        return Status::Error;
    }
    if (updates_.empty()) {
        return lu_.solve_transpose(c_B, y);
    }

    // 1. Initialize w = c_B
    std::vector<Float64> w(c_B.begin(), c_B.end());

    // 2. Apply Eta transpose inverses in reverse order: w <- E_k^-T w, ..., w <- E_1^-T w
    for (Int64 t = static_cast<Int64>(updates_.size()) - 1; t >= 0; --t) {
        const auto& eta = updates_[t];
        Index p = eta.slot_p;
        Float64 S = 0.0;
        for (std::size_t k = 0; k < eta.nz_rows.size(); ++k) {
            Index r = eta.nz_rows[k];
            S += eta.nz_vals[k] * w[r];
        }
        w[p] = (w[p] - S) / eta.alpha_p;
    }

    // 3. Solve base LU transpose system: (B^(0))^T y = w
    return lu_.solve_transpose(w, y);
}

void BasisMatrix::extract_basis_column(const linalg::CSCMatrix& A, const Basis& basis, Index slot, std::span<Float64> col_out) const {
    if (slot < 0 || slot >= m_ || col_out.size() != static_cast<std::size_t>(m_)) {
        throw std::out_of_range("Invalid slot or output size in extract_basis_column");
    }
    std::fill(col_out.begin(), col_out.end(), 0.0);

    Index orig_col = basis.get_basic_var(slot);
    auto r_indices = A.get_col_row_indices(orig_col);
    auto c_values  = A.get_col_values(orig_col);

    for (std::size_t k = 0; k < r_indices.size(); ++k) {
        col_out[r_indices[k]] = c_values[k];
    }
}

} // namespace sovereign::lp
