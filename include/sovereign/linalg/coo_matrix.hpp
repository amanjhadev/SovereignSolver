#ifndef SOVEREIGN_LINALG_COO_MATRIX_HPP
#define SOVEREIGN_LINALG_COO_MATRIX_HPP

#include "sovereign/core/types.hpp"
#include <span>
#include <vector>

namespace sovereign::linalg {

class CSCMatrix;
class CSRMatrix;

class COOMatrix {
private:
    Index rows_{0};
    Index cols_{0};
    std::vector<Index> row_indices_;
    std::vector<Index> col_indices_;
    std::vector<Float64> values_;

public:
    COOMatrix() = default;
    COOMatrix(Index rows, Index cols, std::size_t initial_capacity = 0);

    void resize_dimensions(Index rows, Index cols) noexcept;
    void add_entry(Index row, Index col, Float64 value);
    void clear() noexcept;

    Index num_rows() const noexcept { return rows_; }
    Index num_cols() const noexcept { return cols_; }
    std::size_t num_nonzeros() const noexcept { return values_.size(); }

    std::span<const Index> row_indices() const noexcept { return row_indices_; }
    std::span<const Index> col_indices() const noexcept { return col_indices_; }
    std::span<const Float64> values() const noexcept { return values_; }

    void reserve(std::size_t capacity);

    CSCMatrix to_csc() const;
    CSRMatrix to_csr() const;
};

} // namespace sovereign::linalg

#endif // SOVEREIGN_LINALG_COO_MATRIX_HPP
