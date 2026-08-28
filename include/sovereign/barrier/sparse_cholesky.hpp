#ifndef SOVEREIGN_BARRIER_SPARSE_CHOLESKY_HPP
#define SOVEREIGN_BARRIER_SPARSE_CHOLESKY_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include <span>
#include <vector>

namespace sovereign::barrier {

class SparseCholesky {
private:
    Index n_{0};
    bool is_factored_{false};
    Float64 regularization_used_{0.0};
    
    // Factorization L stored in dense/sparse format for row/col access
    std::vector<Float64> L_dense_; // Size n_ * n_
    std::vector<Float64> M_dense_; // Original M for iterative refinement

public:
    SparseCholesky() = default;

    // Symbolic analysis / dimension setup
    Status analyze_pattern(Index n);

    // Factorizes M = A D^2 A^T + reg * I
    // Automatically increases regularization if non-positive pivot encountered
    Status factorize(const linalg::CSCMatrix& A, std::span<const Float64> d_sq, Float64 min_pivot, Float64 initial_reg, Float64 max_reg);

    // Solves M * x = b using L * L^T x = b, followed by iterative refinement
    Status solve(std::span<const Float64> b, std::span<Float64> x) const;

    // Refines x with iterative refinement against original M
    Status refine(std::span<const Float64> b, std::span<Float64> x) const;

    bool is_factored() const noexcept { return is_factored_; }
    Float64 regularization_used() const noexcept { return regularization_used_; }
    Index dimension() const noexcept { return n_; }
};

} // namespace sovereign::barrier

#endif // SOVEREIGN_BARRIER_SPARSE_CHOLESKY_HPP
