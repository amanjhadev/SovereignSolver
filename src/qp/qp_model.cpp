#include "sovereign/qp/qp_model.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace sovereign::qp {

void QPModel::add_quadratic_term(Index var1, Index var2, Float64 val) {
    if (var1 < 0 || var1 >= num_variables() || var2 < 0 || var2 >= num_variables()) {
        return;
    }
    Index i = std::min(var1, var2);
    Index j = std::max(var1, var2);
    q_entries_.push_back({i, j, val});
    csc_dirty_ = true;
}

void QPModel::rebuild_csc_if_needed() const {
    if (!csc_dirty_) return;
    Index n = num_variables();
    linalg::COOMatrix coo(n, n);
    for (const auto& entry : q_entries_) {
        coo.add_entry(entry.i, entry.j, entry.val);
        if (entry.i != entry.j) {
            coo.add_entry(entry.j, entry.i, entry.val);
        }
    }
    q_csc_ = coo.to_csc();
    csc_dirty_ = false;
}

const linalg::CSCMatrix& QPModel::get_q_csc() const {
    rebuild_csc_if_needed();
    return q_csc_;
}

void QPModel::Q_multiply(std::span<const Float64> x, std::span<Float64> Qx) const {
    std::fill(Qx.begin(), Qx.end(), 0.0);
    for (const auto& entry : q_entries_) {
        if (entry.i == entry.j) {
            Qx[entry.i] += entry.val * x[entry.i];
        } else {
            Qx[entry.i] += entry.val * x[entry.j];
            Qx[entry.j] += entry.val * x[entry.i];
        }
    }
}

Float64 QPModel::evaluate_objective(std::span<const Float64> x) const {
    Float64 val = lp_model_.objective_offset();
    Index n = num_variables();
    for (Index j = 0; j < n; ++j) {
        val += lp_model_.get_variable(j).obj_coeff * x[j];
    }
    for (const auto& entry : q_entries_) {
        if (entry.i == entry.j) {
            val += 0.5 * entry.val * x[entry.i] * x[entry.i];
        } else {
            val += entry.val * x[entry.i] * x[entry.j];
        }
    }
    return val;
}

void QPModel::evaluate_gradient(std::span<const Float64> x, std::span<Float64> grad) const {
    Index n = num_variables();
    for (Index j = 0; j < n; ++j) {
        grad[j] = lp_model_.get_variable(j).obj_coeff;
    }
    std::vector<Float64> Qx(n, 0.0);
    Q_multiply(x, Qx);
    for (Index j = 0; j < n; ++j) {
        grad[j] += Qx[j];
    }
}

QPStatus QPModel::validate_convexity() const {
    Index n = num_variables();
    if (n == 0) return QPStatus::NotSolved;

    // Build dense N x N symmetric Q matrix for convexity validation
    std::vector<Float64> Q_dense(static_cast<std::size_t>(n) * n, 0.0);
    for (const auto& entry : q_entries_) {
        Q_dense[entry.i * n + entry.j] += entry.val;
        if (entry.i != entry.j) {
            Q_dense[entry.j * n + entry.i] += entry.val;
        }
    }

    // Check diagonal entries for negative values
    for (Index i = 0; i < n; ++i) {
        if (Q_dense[i * n + i] < -1e-10) {
            return QPStatus::NonConvexModel;
        }
    }

    // LDL^T pivot checks on Q_dense
    std::vector<Float64> A_work = Q_dense;
    for (Index i = 0; i < n; ++i) {
        Float64 d_ii = A_work[i * n + i];
        for (Index k = 0; k < i; ++k) {
            Float64 l_ik = A_work[i * n + k];
            Float64 d_kk = A_work[k * n + k];
            d_ii -= l_ik * l_ik * d_kk;
        }

        if (d_ii < -1e-10) {
            return QPStatus::NonConvexModel; // Negative pivot -> Indefinite Q!
        }

        A_work[i * n + i] = d_ii;

        if (std::abs(d_ii) > 1e-14) {
            for (Index j = i + 1; j < n; ++j) {
                Float64 sum = A_work[j * n + i];
                for (Index k = 0; k < i; ++k) {
                    sum -= A_work[j * n + k] * A_work[i * n + k] * A_work[k * n + k];
                }
                A_work[j * n + i] = sum / d_ii;
            }
        }
    }

    return QPStatus::NotSolved; // Convex PSD matrix
}

} // namespace sovereign::qp
