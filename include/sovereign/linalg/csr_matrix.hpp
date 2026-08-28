#ifndef SOVEREIGN_LINALG_CSR_MATRIX_HPP
#define SOVEREIGN_LINALG_CSR_MATRIX_HPP

#include "sovereign/core/types.hpp"
#include <span>
#include <vector>

namespace sovereign::linalg {

class COOMatrix;
class CSCMatrix;

class CSRMatrix {
private:
    Index rows_{0};
    Index cols_{0};
    std::vector<Index> row_ptr_; // Size: rows_ + 1
    std::vector<Index> col_idx_; // Size: num_nonzeros
    std::vector<Float64> values_; // Size: num_nonzeros

public:
    CSRMatrix() = default;
    CSRMatrix(Index rows, Index cols, std::vector<Index> row_ptr, std::vector<Index> col_idx, std::vector<Float64> values);

    Index num_rows() const noexcept { return rows_; }
    Index num_cols() const noexcept { return cols_; }
    std::size_t num_nonzeros() const noexcept { return values_.size(); }
    bool is_square() const noexcept { return rows_ == cols_; }

    std::span<const Index> row_ptr() const noexcept { return row_ptr_; }
    std::span<const Index> col_idx() const noexcept { return col_idx_; }
    std::span<const Float64> values() const noexcept { return values_; }

    // Row Access
    std::span<const Index> get_row_col_indices(Index row) const;
    std::span<const Float64> get_row_values(Index row) const;
    Index get_row_nnz(Index row) const;

    // SpMV Operations: y = A * x, y = A^T * x
    void multiply(std::span<const Float64> x, std::span<Float64> y) const;
    void multiply_transpose(std::span<const Float64> x, std::span<Float64> y) const;

    // Matrix Norms
    Float64 infinity_norm() const;
    Float64 one_norm() const;

    // Conversions
    COOMatrix to_coo() const;
    CSCMatrix to_csc() const;

    // Validation
    bool is_valid() const noexcept;
};

} // namespace sovereign::linalg

#endif // SOVEREIGN_LINALG_CSR_MATRIX_HPP
