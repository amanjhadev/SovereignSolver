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

Status BasisMatrix::solve_primal(std::span<const Float64> b, std::span<Float64> x) const {
    if (!is_factored_) {
        return Status::Error;
    }
    return lu_.solve(b, x);
}

Status BasisMatrix::solve_dual(std::span<const Float64> c_B, std::span<Float64> y) const {
    if (!is_factored_) {
        return Status::Error;
    }
    return lu_.solve_transpose(c_B, y);
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
