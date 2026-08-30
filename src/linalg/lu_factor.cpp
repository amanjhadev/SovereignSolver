#include "sovereign/linalg/lu_factor.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/core/vector_ops.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <iostream>

namespace sovereign::linalg {

// ============================================================================
// 1. ReferenceDenseLU Implementation (Oracle)
// ============================================================================

Status ReferenceDenseLU::factorize(const CSCMatrix& A, const SolverConfig& config) {
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

    min_pivot_magnitude_ = INFINITY_VAL;
    max_pivot_magnitude_ = 0.0;
    fill_in_count_ = 0;

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

    std::vector<Float64> u_diags(n_, 0.0);
    struct MultiplierEntry { Index orig_row; Float64 mult; };
    struct UpperEntry { Index orig_col; Float64 val; };
    std::vector<std::vector<MultiplierEntry>> step_l_entries(n_);
    std::vector<std::vector<UpperEntry>> step_u_entries(n_);

    Float64 max_orig_entry = A.infinity_norm();

    for (Index step = 0; step < n_; ++step) {
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

        if (best_row == -1 || std::abs(best_pivot_val) < config.pivot_tolerance) {
            status_ = Status::SingularMatrix;
            is_factored_ = false;
            return status_;
        }

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

    COOMatrix L_coo(n_, n_);
    COOMatrix U_coo(n_, n_);

    for (Index step = 0; step < n_; ++step) {
        L_coo.add_entry(step, step, 1.0);
        for (const auto& l_entry : step_l_entries[step]) {
            Index perm_r = p_row_inv_[l_entry.orig_row];
            L_coo.add_entry(perm_r, step, l_entry.mult);
        }

        U_coo.add_entry(step, step, u_diags[step]);
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

Status ReferenceDenseLU::solve(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) return Status::Error;
    if (b.size() != static_cast<std::size_t>(n_) || x.size() != static_cast<std::size_t>(n_)) {
        return Status::DimensionMismatch;
    }

    std::vector<Float64> pb(n_);
    for (Index k = 0; k < n_; ++k) {
        pb[k] = b[p_row_[k]];
    }

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

        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            if (r < j) {
                w[r] -= vals[k] * z[j];
            }
        }
    }

    for (Index k = 0; k < n_; ++k) {
        x[p_col_[k]] = z[k];
    }

    return Status::Ok;
}

Status ReferenceDenseLU::solve_transpose(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) return Status::Error;
    if (b.size() != static_cast<std::size_t>(n_) || x.size() != static_cast<std::size_t>(n_)) {
        return Status::DimensionMismatch;
    }

    std::vector<Float64> qb(n_);
    for (Index k = 0; k < n_; ++k) {
        qb[k] = b[p_col_[k]];
    }

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

    for (Index k = 0; k < n_; ++k) {
        x[p_row_[k]] = v[k];
    }

    return Status::Ok;
}

// ============================================================================
// 2. True SparseLU Implementation (Node-Pool + SPA + Degree Buckets)
// ============================================================================

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

    min_pivot_magnitude_ = INFINITY_VAL;
    max_pivot_magnitude_ = 0.0;
    fill_in_count_ = 0;
    pivot_candidates_inspected_ = 0;
    pivots_rejected_ = 0;

    Float64 max_orig_entry = A.infinity_norm();

    // Node Pool and Structural Headers
    std::vector<SparseLUNode> pool;
    pool.reserve(A.num_nonzeros() * 3 + 100);

    std::vector<Index> row_head(n_, -1);
    std::vector<Index> col_head(n_, -1);
    std::vector<Index> row_len(n_, 0);
    std::vector<Index> col_len(n_, 0);
    std::vector<Float64> col_max(n_, 0.0);

    // Degree Buckets
    std::vector<Index> head_degree_row(n_ + 1, -1);
    std::vector<Index> next_degree_row(n_, -1);
    std::vector<Index> prev_degree_row(n_, -1);

    std::vector<Index> head_degree_col(n_ + 1, -1);
    std::vector<Index> next_degree_col(n_, -1);
    std::vector<Index> prev_degree_col(n_, -1);

    std::vector<bool> row_active(n_, true);
    std::vector<bool> col_active(n_, true);

    auto link_row_degree = [&](Index r) {
        Index d = row_len[r];
        if (d < 0) d = 0;
        if (d > n_) d = n_;
        Index old_head = head_degree_row[d];
        next_degree_row[r] = old_head;
        prev_degree_row[r] = -1;
        if (old_head != -1) {
            prev_degree_row[old_head] = r;
        }
        head_degree_row[d] = r;
    };

    auto unlink_row_degree = [&](Index r, Index d) {
        if (d < 0) d = 0;
        if (d > n_) d = n_;
        Index prv = prev_degree_row[r];
        Index nxt = next_degree_row[r];
        if (prv != -1) {
            next_degree_row[prv] = nxt;
        } else {
            head_degree_row[d] = nxt;
        }
        if (nxt != -1) {
            prev_degree_row[nxt] = prv;
        }
        next_degree_row[r] = -1;
        prev_degree_row[r] = -1;
    };

    auto link_col_degree = [&](Index c) {
        Index d = col_len[c];
        if (d < 0) d = 0;
        if (d > n_) d = n_;
        Index old_head = head_degree_col[d];
        next_degree_col[c] = old_head;
        prev_degree_col[c] = -1;
        if (old_head != -1) {
            prev_degree_col[old_head] = c;
        }
        head_degree_col[d] = c;
    };

    auto unlink_col_degree = [&](Index c, Index d) {
        if (d < 0) d = 0;
        if (d > n_) d = n_;
        Index prv = prev_degree_col[c];
        Index nxt = next_degree_col[c];
        if (prv != -1) {
            next_degree_col[prv] = nxt;
        } else {
            head_degree_col[d] = nxt;
        }
        if (nxt != -1) {
            prev_degree_col[nxt] = prv;
        }
        next_degree_col[c] = -1;
        prev_degree_col[c] = -1;
    };

    // Helper: Add entry to node pool and update headers/buckets
    auto add_pool_entry = [&](Index r, Index c, Float64 val, bool in_elimination = false) {
        if (std::abs(val) <= config.zero_tolerance) return;

        if (in_elimination) {
            unlink_row_degree(r, row_len[r]);
            if (col_len[c] > 0) {
                unlink_col_degree(c, col_len[c]);
            }
        }

        Index node_idx = static_cast<Index>(pool.size());
        SparseLUNode node;
        node.row = r;
        node.col = c;
        node.val = val;
        node.active = true;

        // Link into row
        node.next_in_row = row_head[r];
        if (row_head[r] != -1) {
            pool[row_head[r]].prev_in_row = node_idx;
        }
        row_head[r] = node_idx;

        // Link into col
        node.next_in_col = col_head[c];
        if (col_head[c] != -1) {
            pool[col_head[c]].prev_in_col = node_idx;
        }
        col_head[c] = node_idx;

        pool.push_back(node);

        row_len[r]++;
        col_len[c]++;
        col_max[c] = std::max(col_max[c], std::abs(val));

        if (in_elimination) {
            link_row_degree(r);
            link_col_degree(c);
        }
    };

    auto get_active_col_max = [&](Index c) {
        Float64 cmax = 0.0;
        Index curr = col_head[c];
        while (curr != -1) {
            const auto& nd = pool[curr];
            if (nd.active && row_active[nd.row]) {
                cmax = std::max(cmax, std::abs(nd.val));
            }
            curr = nd.next_in_col;
        }
        return cmax;
    };

    // Import CSC Matrix A into Node Pool
    for (Index j = 0; j < n_; ++j) {
        auto r_idx = A.get_col_row_indices(j);
        auto vals = A.get_col_values(j);
        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            add_pool_entry(r_idx[k], j, vals[k], false);
        }
    }

    // Initialize Degree Buckets
    for (Index i = 0; i < n_; ++i) link_row_degree(i);
    for (Index j = 0; j < n_; ++j) link_col_degree(j);

    p_row_.assign(n_, -1);
    p_row_inv_.assign(n_, -1);
    p_col_.assign(n_, -1);
    p_col_inv_.assign(n_, -1);

    // Intermediate Factor Storage
    std::vector<Float64> u_diags(n_, 0.0);
    struct MultiplierEntry { Index orig_row; Float64 mult; };
    struct UpperEntry { Index orig_col; Float64 val; };
    std::vector<std::vector<MultiplierEntry>> step_l_entries(n_);
    std::vector<std::vector<UpperEntry>> step_u_entries(n_);

    // SPA Workspace
    std::vector<Float64> spa_val(n_, 0.0);
    std::vector<bool> spa_tok(n_, false);
    std::vector<Index> spa_idx;
    spa_idx.reserve(n_);

    // Pass 1: Elimination Loop
    for (Index step = 0; step < n_; ++step) {
        Index best_row = -1;
        Index best_col = -1;
        Int64 min_markowitz = std::numeric_limits<Int64>::max();
        Float64 best_pivot_val = 0.0;

        Index candidates_inspected = 0;
        const Index k_max_budget = 16;
        bool found_candidate = false;

        // 1. Bucket Candidate Inspection Loop
        for (Index deg = 1; deg <= n_; ++deg) {
            Index curr_r = head_degree_row[deg];
            while (curr_r != -1) {
                if (!row_active[curr_r]) {
                    curr_r = next_degree_row[curr_r];
                    continue;
                }

                candidates_inspected++;
                pivot_candidates_inspected_++;

                // Inspect nonzeros in candidate row curr_r
                Index curr_node_idx = row_head[curr_r];
                while (curr_node_idx != -1) {
                    const auto& nd = pool[curr_node_idx];
                    if (nd.active && col_active[nd.col]) {
                        Index c = nd.col;
                        Float64 val = std::abs(nd.val);
                        Float64 threshold = config.pivot_threshold * get_active_col_max(c);

                        if (val >= threshold && val > config.pivot_tolerance) {
                            Int64 mk_score = static_cast<Int64>(row_len[curr_r] - 1) * static_cast<Int64>(col_len[c] - 1);
                            if (mk_score < min_markowitz || (mk_score == min_markowitz && val > std::abs(best_pivot_val))) {
                                min_markowitz = mk_score;
                                best_row = curr_r;
                                best_col = c;
                                best_pivot_val = nd.val;
                                found_candidate = true;

                                // Early exit check: score <= (deg-1)^2
                                if (mk_score <= static_cast<Int64>(deg - 1) * static_cast<Int64>(deg - 1)) {
                                    break;
                                }
                            }
                        } else {
                            pivots_rejected_++;
                        }
                    }
                    curr_node_idx = nd.next_in_row;
                }

                if (found_candidate && min_markowitz <= static_cast<Int64>(deg - 1) * static_cast<Int64>(deg - 1)) {
                    break;
                }

                if (candidates_inspected >= k_max_budget && found_candidate) {
                    break;
                }

                curr_r = next_degree_row[curr_r];
            }

            if (found_candidate && (min_markowitz <= static_cast<Int64>(deg - 1) * static_cast<Int64>(deg - 1) || candidates_inspected >= k_max_budget)) {
                break;
            }
        }

        // 2. Safe Fallback Search if Budget Failed
        if (!found_candidate) {
            for (Index r = 0; r < n_; ++r) {
                if (!row_active[r]) continue;
                Index curr_node_idx = row_head[r];
                while (curr_node_idx != -1) {
                    const auto& nd = pool[curr_node_idx];
                    if (nd.active && col_active[nd.col]) {
                        Index c = nd.col;
                        Float64 val = std::abs(nd.val);
                        Float64 threshold = config.pivot_threshold * get_active_col_max(c);

                        if (val >= threshold && val > config.pivot_tolerance) {
                            Int64 mk_score = static_cast<Int64>(row_len[r] - 1) * static_cast<Int64>(col_len[c] - 1);
                            if (mk_score < min_markowitz || (mk_score == min_markowitz && val > std::abs(best_pivot_val))) {
                                min_markowitz = mk_score;
                                best_row = r;
                                best_col = c;
                                best_pivot_val = nd.val;
                                found_candidate = true;
                            }
                        }
                    }
                    curr_node_idx = nd.next_in_row;
                }
            }
        }

        // Singularity Check
        if (!found_candidate || std::abs(best_pivot_val) < config.pivot_tolerance) {
            status_ = Status::SingularMatrix;
            is_factored_ = false;
            return status_;
        }

        // Record Pivot Permutations
        p_row_[step] = best_row;
        p_col_[step] = best_col;
        p_row_inv_[best_row] = step;
        p_col_inv_[best_col] = step;

        unlink_row_degree(best_row, row_len[best_row]);
        unlink_col_degree(best_col, col_len[best_col]);
        row_active[best_row] = false;
        col_active[best_col] = false;

        Float64 abs_pivot = std::abs(best_pivot_val);
        min_pivot_magnitude_ = std::min(min_pivot_magnitude_, abs_pivot);
        max_pivot_magnitude_ = std::max(max_pivot_magnitude_, abs_pivot);
        u_diags[step] = best_pivot_val;

        // Record U Row Entries from Pivot Row best_row
        Index pivot_row_node = row_head[best_row];
        while (pivot_row_node != -1) {
            const auto& nd = pool[pivot_row_node];
            if (nd.active && col_active[nd.col] && nd.col != best_col) {
                step_u_entries[step].push_back({nd.col, nd.val});
            }
            pivot_row_node = nd.next_in_row;
        }

        // Elimination via SPA
        Index target_col_node = col_head[best_col];
        std::vector<Index> target_rows;
        while (target_col_node != -1) {
            const auto& nd = pool[target_col_node];
            if (nd.active && row_active[nd.row] && nd.row != best_row) {
                target_rows.push_back(nd.row);
            }
            target_col_node = nd.next_in_col;
        }

        for (Index target_r : target_rows) {
            Float64 a_iq = 0.0;
            Index node_q = row_head[target_r];
            while (node_q != -1) {
                if (pool[node_q].active && pool[node_q].col == best_col) {
                    a_iq = pool[node_q].val;
                    break;
                }
                node_q = pool[node_q].next_in_row;
            }

            Float64 mult = a_iq / best_pivot_val;
            if (std::abs(mult) <= config.zero_tolerance) continue;

            step_l_entries[step].push_back({target_r, mult});

            // 1. Load target row target_r into SPA
            Index curr = row_head[target_r];
            while (curr != -1) {
                const auto& nd = pool[curr];
                if (nd.active && col_active[nd.col] && nd.col != best_col) {
                    spa_val[nd.col] = nd.val;
                    spa_tok[nd.col] = true;
                    spa_idx.push_back(nd.col);
                }
                curr = nd.next_in_row;
            }

            // 2. Perform Sparse Linear Combination with Pivot Row
            Index pnode = row_head[best_row];
            while (pnode != -1) {
                const auto& nd = pool[pnode];
                if (nd.active && col_active[nd.col] && nd.col != best_col) {
                    Index j = nd.col;
                    Float64 new_v = spa_val[j] - mult * nd.val;
                    if (spa_tok[j]) {
                        spa_val[j] = new_v;
                    } else {
                        spa_val[j] = new_v;
                        spa_tok[j] = true;
                        spa_idx.push_back(j);
                    }
                }
                pnode = nd.next_in_row;
            }

            // 3. Clear existing active entries in row target_r and update col_len & col_degree
            curr = row_head[target_r];
            while (curr != -1) {
                if (pool[curr].active && col_active[pool[curr].col]) {
                    pool[curr].active = false;
                    Index c = pool[curr].col;
                    unlink_col_degree(c, col_len[c]);
                    col_len[c]--;
                    if (col_len[c] > 0) {
                        link_col_degree(c);
                    }
                }
                curr = pool[curr].next_in_row;
            }

            unlink_row_degree(target_r, row_len[target_r]);
            row_head[target_r] = -1;
            row_len[target_r] = 0;

            // 4. Scatter SPA nonzeros back to Node Pool for row target_r
            for (Index c : spa_idx) {
                Float64 v = spa_val[c];
                if (std::abs(v) > config.zero_tolerance) {
                    add_pool_entry(target_r, c, v, true);
                }
                spa_val[c] = 0.0;
                spa_tok[c] = false;
            }
            spa_idx.clear();
        }
    }

    // Pass 2: Direct CSC Factor Assembly
    COOMatrix L_coo(n_, n_);
    COOMatrix U_coo(n_, n_);

    for (Index step = 0; step < n_; ++step) {
        L_coo.add_entry(step, step, 1.0);
        for (const auto& l_entry : step_l_entries[step]) {
            Index perm_r = p_row_inv_[l_entry.orig_row];
            L_coo.add_entry(perm_r, step, l_entry.mult);
        }

        U_coo.add_entry(step, step, u_diags[step]);
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
    if (!is_factored_) return Status::Error;
    if (b.size() != static_cast<std::size_t>(n_) || x.size() != static_cast<std::size_t>(n_)) {
        return Status::DimensionMismatch;
    }

    // P * A * Q = L * U => A = P^T * L * U * Q^T
    // 1. pb = P * b
    std::vector<Float64> pb(n_);
    for (Index k = 0; k < n_; ++k) {
        pb[k] = b[p_row_[k]];
    }

    // 2. L * w = pb
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

    // 3. U * z = w
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

        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            if (r < j) {
                w[r] -= vals[k] * z[j];
            }
        }
    }

    // 4. x = Q * z => x[p_col_[k]] = z[k]
    for (Index k = 0; k < n_; ++k) {
        x[p_col_[k]] = z[k];
    }

    return Status::Ok;
}

Status SparseLU::solve_transpose(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) return Status::Error;
    if (b.size() != static_cast<std::size_t>(n_) || x.size() != static_cast<std::size_t>(n_)) {
        return Status::DimensionMismatch;
    }

    // A^T * x = b => Q * U^T * L^T * P * x = b => U^T * L^T * P * x = Q^T * b
    // 1. qb = Q^T * b
    std::vector<Float64> qb(n_);
    for (Index k = 0; k < n_; ++k) {
        qb[k] = b[p_col_[k]];
    }

    // 2. U^T * y = qb
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

    // 3. L^T * v = y
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

    // 4. P * x = v => x[p_row_[k]] = v[k]
    for (Index k = 0; k < n_; ++k) {
        x[p_row_[k]] = v[k];
    }

    return Status::Ok;
}

} // namespace sovereign::linalg
