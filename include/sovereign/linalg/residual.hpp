#ifndef SOVEREIGN_LINALG_RESIDUAL_HPP
#define SOVEREIGN_LINALG_RESIDUAL_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/lu_factor.hpp"
#include <span>

namespace sovereign::linalg {

struct ResidualStats {
    Float64 absolute_residual_inf{0.0};
    Float64 absolute_residual_l2{0.0};
    Float64 relative_residual_inf{0.0};
};

// Computes residual vector r = b - A * x
void compute_residual(const CSCMatrix& A, std::span<const Float64> x, std::span<const Float64> b, std::span<Float64> r);

// Evaluates absolute and relative residual norms
ResidualStats evaluate_residual(const CSCMatrix& A, std::span<const Float64> x, std::span<const Float64> b);

// Iterative Refinement: Corrects solution x via residual system solve A * delta = b - A * x
Status iterative_refinement(const CSCMatrix& A, const SparseLU& lu, std::span<const Float64> b, std::span<Float64> x, Int32 max_iters = 5, Float64 target_tol = 1e-12);

} // namespace sovereign::linalg

#endif // SOVEREIGN_LINALG_RESIDUAL_HPP
