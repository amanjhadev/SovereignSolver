#ifndef SOVEREIGN_LINALG_LU_FACTOR_HPP
#define SOVEREIGN_LINALG_LU_FACTOR_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/csr_matrix.hpp"
#include <span>
#include <vector>
#include <cstddef>

namespace sovereign::linalg {

// Node representation for sparse linked-structure factorizer
struct SparseLUNode {
    Index row{-1};
    Index col{-1};
    Float64 val{0.0};
    Index next_in_row{-1};
    Index prev_in_row{-1};
    Index next_in_col{-1};
    Index prev_in_col{-1};
    bool active{false};
};

// Reference Oracle: Original Dense Working LU Factorization
class ReferenceDenseLU {
private:
    Index n_{0};
    bool is_factored_{false};
    Status status_{Status::Ok};

    std::vector<Index> p_row_;
    std::vector<Index> p_row_inv_;
    std::vector<Index> p_col_;
    std::vector<Index> p_col_inv_;

    CSCMatrix L_;
    CSCMatrix U_;

    Float64 min_pivot_magnitude_{INFINITY_VAL};
    Float64 max_pivot_magnitude_{0.0};
    Float64 pivot_growth_factor_{1.0};
    std::size_t fill_in_count_{0};

public:
    ReferenceDenseLU() = default;

    Status factorize(const CSCMatrix& A, const SolverConfig& config = SolverConfig::default_config());
    Status solve(std::span<const Float64> b, std::span<Float64> x) const;
    Status solve_transpose(std::span<const Float64> b, std::span<Float64> x) const;

    bool is_factored() const noexcept { return is_factored_; }
    Status status() const noexcept { return status_; }
    Index dimension() const noexcept { return n_; }

    std::span<const Index> row_perm() const noexcept { return p_row_; }
    std::span<const Index> col_perm() const noexcept { return p_col_; }

    const CSCMatrix& L() const noexcept { return L_; }
    const CSCMatrix& U() const noexcept { return U_; }

    Float64 min_pivot() const noexcept { return min_pivot_magnitude_; }
    Float64 max_pivot() const noexcept { return max_pivot_magnitude_; }
    std::size_t fill_in() const noexcept { return fill_in_count_; }
};

// True Sparse LU Factorization Engine (Node-Pool + SPA + Degree Buckets)
class SparseLU {
private:
    Index n_{0};
    bool is_factored_{false};
    Status status_{Status::Ok};

    // Permutation vectors (Size n_)
    std::vector<Index> p_row_;     // P: maps original row to permuted row step k
    std::vector<Index> p_row_inv_; // P^-1
    std::vector<Index> p_col_;     // Q: maps permuted col step k to original col
    std::vector<Index> p_col_inv_; // Q^-1

    // Factorized L and U matrices in CSC format
    CSCMatrix L_; // Unit lower triangular matrix in CSC
    CSCMatrix U_; // Upper triangular matrix in CSC

    // Diagnostic Stats
    Float64 min_pivot_magnitude_{INFINITY_VAL};
    Float64 max_pivot_magnitude_{0.0};
    Float64 pivot_growth_factor_{1.0};
    std::size_t fill_in_count_{0};
    std::size_t pivot_candidates_inspected_{0};
    std::size_t pivots_rejected_{0};

public:
    SparseLU() = default;

    // Factorizes square CSC matrix A into P * A * Q = L * U
    Status factorize(const CSCMatrix& A, const SolverConfig& config = SolverConfig::default_config());

    // Solves A * x = b
    Status solve(std::span<const Float64> b, std::span<Float64> x) const;

    // Solves A^T * x = b
    Status solve_transpose(std::span<const Float64> b, std::span<Float64> x) const;

    // Inspection & Query API
    bool is_factored() const noexcept { return is_factored_; }
    Status status() const noexcept { return status_; }
    Index dimension() const noexcept { return n_; }

    std::span<const Index> row_perm() const noexcept { return p_row_; }
    std::span<const Index> col_perm() const noexcept { return p_col_; }

    const CSCMatrix& L() const noexcept { return L_; }
    const CSCMatrix& U() const noexcept { return U_; }

    Float64 min_pivot() const noexcept { return min_pivot_magnitude_; }
    Float64 max_pivot() const noexcept { return max_pivot_magnitude_; }
    Float64 growth_factor() const noexcept { return pivot_growth_factor_; }
    std::size_t fill_in() const noexcept { return fill_in_count_; }
    std::size_t pivot_candidates() const noexcept { return pivot_candidates_inspected_; }
    std::size_t pivots_rejected() const noexcept { return pivots_rejected_; }
};

} // namespace sovereign::linalg

#endif // SOVEREIGN_LINALG_LU_FACTOR_HPP
