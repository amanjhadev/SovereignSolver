#include "sovereign/linalg/residual.hpp"
#include "sovereign/core/vector_ops.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

namespace sovereign::linalg {

void compute_residual(const CSCMatrix& A, std::span<const Float64> x, std::span<const Float64> b, std::span<Float64> r) {
    // r = b - A * x
    std::vector<Float64> ax(A.num_rows(), 0.0);
    A.multiply(x, ax);
    for (std::size_t i = 0; i < b.size(); ++i) {
        r[i] = b[i] - ax[i];
    }
}

ResidualStats evaluate_residual(const CSCMatrix& A, std::span<const Float64> x, std::span<const Float64> b) {
    std::vector<Float64> r(b.size(), 0.0);
    compute_residual(A, x, b, r);

    ResidualStats stats;
    stats.absolute_residual_inf = infinity_norm(r);
    stats.absolute_residual_l2  = l2_norm(r);

    Float64 norm_a = A.infinity_norm();
    Float64 norm_x = infinity_norm(x);
    Float64 norm_b = infinity_norm(b);

    Float64 denom = norm_a * norm_x + norm_b;
    if (denom > 0.0) {
        stats.relative_residual_inf = stats.absolute_residual_inf / denom;
    } else {
        stats.relative_residual_inf = stats.absolute_residual_inf;
    }

    return stats;
}

Status iterative_refinement(const CSCMatrix& A, const SparseLU& lu, std::span<const Float64> b, std::span<Float64> x, Int32 max_iters, Float64 target_tol) {
    if (!lu.is_factored()) {
        return Status::Error;
    }

    std::vector<Float64> r(b.size(), 0.0);
    std::vector<Float64> delta(b.size(), 0.0);

    for (Int32 iter = 0; iter < max_iters; ++iter) {
        compute_residual(A, x, b, r);
        Float64 res_norm = infinity_norm(r);

        if (res_norm <= target_tol) {
            break;
        }

        Status status = lu.solve(r, delta);
        if (!is_ok(status)) {
            return status;
        }

        Float64 delta_norm = infinity_norm(delta);
        axpy(1.0, delta, x); // x = x + delta

        if (delta_norm <= target_tol * infinity_norm(x)) {
            break;
        }
    }

    return Status::Ok;
}

} // namespace sovereign::linalg
