#include "sovereign/barrier/sparse_cholesky.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace sovereign::barrier {

Status SparseCholesky::analyze_pattern(Index n) {
    if (n < 0) return Status::InvalidInput;
    n_ = n;
    L_dense_.assign(static_cast<std::size_t>(n_) * n_, 0.0);
    M_dense_.assign(static_cast<std::size_t>(n_) * n_, 0.0);
    is_factored_ = false;
    regularization_used_ = 0.0;
    return Status::Ok;
}

Status SparseCholesky::factorize(
    const linalg::CSCMatrix& A,
    std::span<const Float64> d_sq,
    Float64 min_pivot,
    Float64 initial_reg,
    Float64 max_reg
) {
    if (A.num_rows() != n_) {
        analyze_pattern(A.num_rows());
    }

    Index m = n_;
    Index n = A.num_cols();
    if (d_sq.size() != static_cast<std::size_t>(n)) {
        return Status::DimensionMismatch;
    }

    // 1. Build M = A * D^2 * A^T in M_dense_
    std::fill(M_dense_.begin(), M_dense_.end(), 0.0);

    for (Index j = 0; j < n; ++j) {
        Float64 d2 = d_sq[j];
        if (d2 <= 0.0) continue;

        auto r_idx = A.get_col_row_indices(j);
        auto vals = A.get_col_values(j);
        std::size_t nnz_j = r_idx.size();

        for (std::size_t k1 = 0; k1 < nnz_j; ++k1) {
            Index r1 = r_idx[k1];
            Float64 v1 = vals[k1];
            for (std::size_t k2 = 0; k2 < nnz_j; ++k2) {
                Index r2 = r_idx[k2];
                Float64 v2 = vals[k2];
                M_dense_[r1 * m + r2] += v1 * v2 * d2;
            }
        }
    }

    // 2. Adaptive regularization loop
    Float64 reg = initial_reg;
    bool success = false;

    while (reg <= max_reg * 10.0) {
        std::copy(M_dense_.begin(), M_dense_.end(), L_dense_.begin());

        // Add diagonal regularization
        for (Index i = 0; i < m; ++i) {
            Float64 diag_scale = std::max(1.0, std::abs(M_dense_[i * m + i]));
            L_dense_[i * m + i] += reg * diag_scale;
        }

        // Cholesky L * L^T factorization
        bool piv_ok = true;
        for (Index i = 0; i < m; ++i) {
            Float64 sum = L_dense_[i * m + i];
            for (Index k = 0; k < i; ++k) {
                Float64 lik = L_dense_[i * m + k];
                sum -= lik * lik;
            }

            if (sum <= min_pivot) {
                piv_ok = false;
                break;
            }

            Float64 l_ii = std::sqrt(sum);
            L_dense_[i * m + i] = l_ii;

            for (Index j = i + 1; j < m; ++j) {
                Float64 sum_j = L_dense_[j * m + i];
                for (Index k = 0; k < i; ++k) {
                    sum_j -= L_dense_[j * m + k] * L_dense_[i * m + k];
                }
                L_dense_[j * m + i] = sum_j / l_ii;
                L_dense_[i * m + j] = 0.0; // Upper triangle set to 0
            }
        }

        if (piv_ok) {
            success = true;
            regularization_used_ = reg;
            break;
        }

        if (reg == 0.0) reg = initial_reg;
        else reg *= 10.0;
    }

    if (!success) {
        is_factored_ = false;
        return Status::NumericalInstability;
    }

    is_factored_ = true;
    return Status::Ok;
}

Status SparseCholesky::solve(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) return Status::Error;
    if (b.size() != static_cast<std::size_t>(n_) || x.size() != static_cast<std::size_t>(n_)) {
        return Status::DimensionMismatch;
    }

    Index m = n_;
    std::vector<Float64> y(m, 0.0);

    // 1. Forward solve L * y = b
    for (Index i = 0; i < m; ++i) {
        Float64 sum = b[i];
        for (Index k = 0; k < i; ++k) {
            sum -= L_dense_[i * m + k] * y[k];
        }
        Float64 l_ii = L_dense_[i * m + i];
        if (std::abs(l_ii) < 1e-15) return Status::NumericalInstability;
        y[i] = sum / l_ii;
    }

    // 2. Backward solve L^T * x = y
    for (Index i = m - 1; i >= 0; --i) {
        Float64 sum = y[i];
        for (Index k = i + 1; k < m; ++k) {
            sum -= L_dense_[k * m + i] * x[k];
        }
        Float64 l_ii = L_dense_[i * m + i];
        if (std::abs(l_ii) < 1e-15) return Status::NumericalInstability;
        x[i] = sum / l_ii;
    }

    // 3. Perform iterative refinement
    refine(b, x);

    return Status::Ok;
}

Status SparseCholesky::refine(std::span<const Float64> b, std::span<Float64> x) const {
    Index m = n_;
    std::vector<Float64> r(m, 0.0);

    for (int iter = 0; iter < 2; ++iter) {
        // Calculate r = b - M_dense_ * x
        Float64 max_r = 0.0;
        for (Index i = 0; i < m; ++i) {
            Float64 sum = 0.0;
            for (Index j = 0; j < m; ++j) {
                sum += M_dense_[i * m + j] * x[j];
            }
            r[i] = b[i] - sum;
            max_r = std::max(max_r, std::abs(r[i]));
        }

        if (max_r < 1e-10) break; // Residual sufficiently small

        // Solve L * L^T * dx = r
        std::vector<Float64> y_r(m, 0.0);
        std::vector<Float64> dx(m, 0.0);

        for (Index i = 0; i < m; ++i) {
            Float64 sum = r[i];
            for (Index k = 0; k < i; ++k) {
                sum -= L_dense_[i * m + k] * y_r[k];
            }
            y_r[i] = sum / L_dense_[i * m + i];
        }

        for (Index i = m - 1; i >= 0; --i) {
            Float64 sum = y_r[i];
            for (Index k = i + 1; k < m; ++k) {
                sum -= L_dense_[k * m + i] * dx[k];
            }
            dx[i] = sum / L_dense_[i * m + i];
        }

        for (Index i = 0; i < m; ++i) {
            x[i] += dx[i];
        }
    }

    return Status::Ok;
}

} // namespace sovereign::barrier
