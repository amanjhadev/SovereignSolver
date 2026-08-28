#ifndef SOVEREIGN_LINALG_CSC_MATRIX_HPP
#define SOVEREIGN_LINALG_CSC_MATRIX_HPP

#include "sovereign/core/types.hpp"
#include <span>
#include <vector>
#include <tuple>

namespace sovereign::linalg {

class COOMatrix;
class CSRMatrix;

class CSCMatrix {
private:
    Index rows_{0};
    Index cols_{0};
    std::vector<Index> col_ptr_; // Size: cols_ + 1
    std::vector<Index> row_idx_; // Size: num_nonzeros
    std::vector<Float64> values_; // Size: num_nonzeros

public:
    CSCMatrix() = default;
    CSCMatrix(Index rows, Index cols, std::vector<Index> col_ptr, std::vector<Index> row_idx, std::vector<Float64> values);

    Index num_rows() const noexcept { return rows_; }
    Index num_cols() const noexcept { return cols_; }
    std::size_t num_nonzeros() const noexcept { return values_.size(); }
    bool is_square() const noexcept { return rows_ == cols_; }

    std::span<const Index> col_ptr() const noexcept { return col_ptr_; }
    std::span<const Index> row_idx() const noexcept { return row_idx_; }
    std::span<const Float64> values() const noexcept { return values_; }

    // Column Access
    std::span<const Index> get_col_row_indices(Index col) const;
    std::span<const Float64> get_col_values(Index col) const;
    Index get_col_nnz(Index col) const;

    // SpMV Operations: y = A * x, y = A^T * x
    void multiply(std::span<const Float64> x, std::span<Float64> y) const;
    void multiply_transpose(std::span<const Float64> x, std::span<Float64> y) const;

    // Matrix Norms
    Float64 infinity_norm() const;
    Float64 one_norm() const;

    // Scaling
    void scale_rows(std::span<const Float64> row_scales);
    void scale_cols(std::span<const Float64> col_scales);

    // Conversions
    COOMatrix to_coo() const;
    CSRMatrix to_csr() const;
    CSCMatrix transpose() const;

    // Validation
    bool is_valid() const noexcept;
};

} // namespace sovereign::linalg

#endif // SOVEREIGN_LINALG_CSC_MATRIX_HPP
