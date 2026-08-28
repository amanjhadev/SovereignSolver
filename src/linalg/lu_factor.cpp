#include "sovereign/linalg/lu_factor.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/core/vector_ops.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace sovereign::linalg {

Status SparseLU::factorize(const CSCMatrix& A, const SolverConfig& config) {
    if (!A.is_square()) {
        status_ = Status::DimensionMismatch;
        is_factored_ = false;
        return status_;
    }

    n_ = A.num_rows();
    if (n_ == 0) {
        status_ = Status::Ok;
        is_factored_ = true;
        return status_;
    }

    // Reset statistics
    min_pivot_magnitude_ = INFINITY_VAL;
    max_pivot_magnitude_ = 0.0;
    fill_in_count_ = 0;

    // Working dense table initialized from CSC matrix A
    std::vector<std::vector<Float64>> work(n_, std::vector<Float64>(n_, 0.0));
    for (Index j = 0; j < n_; ++j) {
        auto r_idx = A.get_col_row_indices(j);
        auto vals = A.get_col_values(j);
        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            work[r_idx[k]][j] += vals[k];
        }
    }

    p_row_.assign(n_, -1);
    p_row_inv_.assign(n_, -1);
    p_col_.assign(n_, -1);
    p_col_inv_.assign(n_, -1);

    std::vector<bool> row_used(n_, false);
    std::vector<bool> col_used(n_, false);

    // Intermediate structures for 2-pass exact L and U construction
    std::vector<Float64> u_diags(n_, 0.0);
    struct MultiplierEntry { Index orig_row; Float64 mult; };
    struct UpperEntry { Index orig_col; Float64 val; };
    std::vector<std::vector<MultiplierEntry>> step_l_entries(n_);
    std::vector<std::vector<UpperEntry>> step_u_entries(n_);

    Float64 max_orig_entry = A.infinity_norm();

    // Pass 1: Pivot Selection & Elimination
    for (Index step = 0; step < n_; ++step) {
        // Step 1: Compute active column maximums for threshold pivoting
        std::vector<Float64> col_max(n_, 0.0);
        for (Index j = 0; j < n_; ++j) {
            if (col_used[j]) continue;
            Float64 cmax = 0.0;
            for (Index i = 0; i < n_; ++i) {
                if (row_used[i]) continue;
                cmax = std::max(cmax, std::abs(work[i][j]));
            }
            col_max[j] = cmax;
        }

        // Step 2: Compute Markowitz counts for uneliminated rows/cols
        std::vector<Index> row_nnz(n_, 0);
        std::vector<Index> col_nnz(n_, 0);
        for (Index i = 0; i < n_; ++i) {
            if (row_used[i]) continue;
            for (Index j = 0; j < n_; ++j) {
                if (col_used[j]) continue;
                if (std::abs(work[i][j]) > config.zero_tolerance) {
                    row_nnz[i]++;
                    col_nnz[j]++;
                }
            }
        }

        // Step 3: Select optimal Markowitz pivot satisfying threshold condition
        Index best_row = -1;
        Index best_col = -1;
        Int64 min_markowitz = std::numeric_limits<Int64>::max();
        Float64 best_pivot_val = 0.0;

        for (Index j = 0; j < n_; ++j) {
            if (col_used[j]) continue;
            Float64 threshold = config.pivot_threshold * col_max[j];

            for (Index i = 0; i < n_; ++i) {
                if (row_used[i]) continue;
                Float64 val = std::abs(work[i][j]);

                if (val >= threshold && val > config.zero_tolerance) {
                    Int64 mk_count = static_cast<Int64>(row_nnz[i] - 1) * static_cast<Int64>(col_nnz[j] - 1);
                    if (mk_count < min_markowitz || (mk_count == min_markowitz && val > std::abs(best_pivot_val))) {
                        min_markowitz = mk_count;
                        best_row = i;
                        best_col = j;
                        best_pivot_val = work[i][j];
                    }
                }
            }
        }

        // Singularity check
        if (best_row == -1 || std::abs(best_pivot_val) < config.pivot_tolerance) {
            status_ = Status::SingularMatrix;
            is_factored_ = false;
            return status_;
        }

        // Record pivot permutations
        p_row_[step] = best_row;
        p_col_[step] = best_col;
        p_row_inv_[best_row] = step;
        p_col_inv_[best_col] = step;

        row_used[best_row] = true;
        col_used[best_col] = true;

        Float64 abs_pivot = std::abs(best_pivot_val);
        min_pivot_magnitude_ = std::min(min_pivot_magnitude_, abs_pivot);
        max_pivot_magnitude_ = std::max(max_pivot_magnitude_, abs_pivot);

        u_diags[step] = best_pivot_val;

        // Step 4: Perform elimination & record multipliers and upper values
        for (Index i = 0; i < n_; ++i) {
            if (row_used[i]) continue;
            Float64 mult = work[i][best_col] / best_pivot_val;

            if (std::abs(mult) > config.zero_tolerance) {
                step_l_entries[step].push_back({i, mult});

                for (Index j = 0; j < n_; ++j) {
                    if (col_used[j]) continue;
                    work[i][j] -= mult * work[best_row][j];
                }
            }
        }

        for (Index j = 0; j < n_; ++j) {
            if (col_used[j]) continue;
            Float64 u_val = work[best_row][j];
            if (std::abs(u_val) > config.zero_tolerance) {
                step_u_entries[step].push_back({j, u_val});
            }
        }
    }

    // Pass 2: Build permuted L and U matrices in CSC format
    COOMatrix L_coo(n_, n_);
    COOMatrix U_coo(n_, n_);

    for (Index step = 0; step < n_; ++step) {
        L_coo.add_entry(step, step, 1.0); // Unit diagonal
        for (const auto& l_entry : step_l_entries[step]) {
            Index perm_r = p_row_inv_[l_entry.orig_row];
            L_coo.add_entry(perm_r, step, l_entry.mult);
        }

        U_coo.add_entry(step, step, u_diags[step]); // Pivot diagonal
        for (const auto& u_entry : step_u_entries[step]) {
            Index perm_c = p_col_inv_[u_entry.orig_col];
            U_coo.add_entry(step, perm_c, u_entry.val);
        }
    }

    L_ = L_coo.to_csc();
    U_ = U_coo.to_csc();

    if (max_orig_entry > 0.0) {
        pivot_growth_factor_ = max_pivot_magnitude_ / max_orig_entry;
    }

    fill_in_count_ = (L_.num_nonzeros() + U_.num_nonzeros()) - A.num_nonzeros();

    status_ = Status::Ok;
    is_factored_ = true;
    return status_;
}

Status SparseLU::solve(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) {
        return Status::Error;
    }
    if (b.size() != static_cast<std::size_t>(n_) || x.size() != static_cast<std::size_t>(n_)) {
        return Status::DimensionMismatch;
    }

    // P * A * Q = L * U => A = P^T * L * U * Q^T
    // A * x = b => P^T * L * U * Q^T * x = b => L * U * (Q^T * x) = P * b
    // 1. pb = P * b: pb[k] = b[p_row_[k]]
    std::vector<Float64> pb(n_);
    for (Index k = 0; k < n_; ++k) {
        pb[k] = b[p_row_[k]];
    }

    // 2. Forward solve L * w = pb (L is CSC unit lower triangular)
    std::vector<Float64> w = pb;
    for (Index j = 0; j < n_; ++j) {
        Float64 wj = w[j];
        if (wj == 0.0) continue;
        auto r_idx = L_.get_col_row_indices(j);
        auto vals = L_.get_col_values(j);
        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            if (r > j) {
                w[r] -= vals[k] * wj;
            }
        }
    }

    // 3. Backward solve U * z = w (U is CSC upper triangular)
    std::vector<Float64> z(n_, 0.0);
    for (Index j = n_ - 1; j >= 0; --j) {
        Float64 diag_val = 0.0;
        auto r_idx = U_.get_col_row_indices(j);
        auto vals = U_.get_col_values(j);

        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            if (r_idx[k] == j) {
                diag_val = vals[k];
                break;
            }
        }

        if (std::abs(diag_val) < ZERO_TOLERANCE) {
            return Status::SingularMatrix;
        }

        z[j] = w[j] / diag_val;

        // Subtract U(r, j) * z[j] from w[r] for all r < j
        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            if (r < j) {
                w[r] -= vals[k] * z[j];
            }
        }
    }

    // 4. Map solution x = Q * z: z = Q^T x => x[p_col_[k]] = z[k]
    for (Index k = 0; k < n_; ++k) {
        x[p_col_[k]] = z[k];
    }

    return Status::Ok;
}

Status SparseLU::solve_transpose(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) {
        return Status::Error;
    }
    if (b.size() != static_cast<std::size_t>(n_) || x.size() != static_cast<std::size_t>(n_)) {
        return Status::DimensionMismatch;
    }

    // A^T * x = b => Q * U^T * L^T * P * x = b => U^T * L^T * (P * x) = Q^T * b
    // 1. qb = Q^T * b => qb[k] = b[p_col_[k]]
    std::vector<Float64> qb(n_);
    for (Index k = 0; k < n_; ++k) {
        qb[k] = b[p_col_[k]];
    }

    // 2. Forward solve U^T * y = qb (U^T is lower triangular with diagonal U(i,i))
    std::vector<Float64> y(n_, 0.0);
    for (Index i = 0; i < n_; ++i) {
        Float64 sum = qb[i];
        Float64 diag_val = 0.0;
        auto r_idx = U_.get_col_row_indices(i);
        auto vals = U_.get_col_values(i);

        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            if (r == i) {
                diag_val = vals[k];
            } else if (r < i) {
                sum -= vals[k] * y[r];
            }
        }

        if (std::abs(diag_val) < ZERO_TOLERANCE) {
            return Status::SingularMatrix;
        }

        y[i] = sum / diag_val;
    }

    // 3. Backward solve L^T * v = y (L^T is upper triangular with unit diagonal)
    std::vector<Float64> v(n_, 0.0);
    for (Index i = n_ - 1; i >= 0; --i) {
        Float64 sum = y[i];
        auto r_idx = L_.get_col_row_indices(i);
        auto vals = L_.get_col_values(i);
        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            if (r > i) {
                sum -= vals[k] * v[r];
            }
        }
        v[i] = sum;
    }

    // 4. Map back P * x = v => (P * x)_k = v_k => x[p_row_[k]] = v[k]
    for (Index k = 0; k < n_; ++k) {
        x[p_row_[k]] = v[k];
    }

    return Status::Ok;
}

} // namespace sovereign::linalg
