#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csr_matrix.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sovereign::linalg {

CSCMatrix::CSCMatrix(Index rows, Index cols, std::vector<Index> col_ptr, std::vector<Index> row_idx, std::vector<Float64> values)
    : rows_(rows), cols_(cols), col_ptr_(std::move(col_ptr)), row_idx_(std::move(row_idx)), values_(std::move(values)) {
    if (col_ptr_.size() != static_cast<std::size_t>(cols_ + 1)) {
        throw std::invalid_argument("CSC col_ptr size must be cols + 1");
    }
}

std::span<const Index> CSCMatrix::get_col_row_indices(Index col) const {
    if (col < 0 || col >= cols_) {
        throw std::out_of_range("Column index out of bounds");
    }
    Index start = col_ptr_[col];
    Index end = col_ptr_[col + 1];
    return std::span<const Index>(row_idx_.data() + start, end - start);
}

std::span<const Float64> CSCMatrix::get_col_values(Index col) const {
    if (col < 0 || col >= cols_) {
        throw std::out_of_range("Column index out of bounds");
    }
    Index start = col_ptr_[col];
    Index end = col_ptr_[col + 1];
    return std::span<const Float64>(values_.data() + start, end - start);
}

Index CSCMatrix::get_col_nnz(Index col) const {
    if (col < 0 || col >= cols_) {
        throw std::out_of_range("Column index out of bounds");
    }
    return col_ptr_[col + 1] - col_ptr_[col];
}

void CSCMatrix::multiply(std::span<const Float64> x, std::span<Float64> y) const {
    if (x.size() != static_cast<std::size_t>(cols_) || y.size() != static_cast<std::size_t>(rows_)) {
        throw std::invalid_argument("SpMV dimension mismatch in CSC multiply");
    }
    std::fill(y.begin(), y.end(), 0.0);
    for (Index j = 0; j < cols_; ++j) {
        Float64 xj = x[j];
        if (xj == 0.0) continue;
        Index start = col_ptr_[j];
        Index end = col_ptr_[j + 1];
        for (Index k = start; k < end; ++k) {
            y[row_idx_[k]] += values_[k] * xj;
        }
    }
}

void CSCMatrix::multiply_transpose(std::span<const Float64> x, std::span<Float64> y) const {
    if (x.size() != static_cast<std::size_t>(rows_) || y.size() != static_cast<std::size_t>(cols_)) {
        throw std::invalid_argument("SpMV dimension mismatch in CSC multiply_transpose");
    }
    for (Index j = 0; j < cols_; ++j) {
        Float64 sum = 0.0;
        Index start = col_ptr_[j];
        Index end = col_ptr_[j + 1];
        for (Index k = start; k < end; ++k) {
            sum += values_[k] * x[row_idx_[k]];
        }
        y[j] = sum;
    }
}

Float64 CSCMatrix::infinity_norm() const {
    std::vector<Float64> row_sums(rows_, 0.0);
    for (Index j = 0; j < cols_; ++j) {
        Index start = col_ptr_[j];
        Index end = col_ptr_[j + 1];
        for (Index k = start; k < end; ++k) {
            row_sums[row_idx_[k]] += std::abs(values_[k]);
        }
    }
    Float64 max_norm = 0.0;
    for (Float64 s : row_sums) {
        max_norm = std::max(max_norm, s);
    }
    return max_norm;
}

Float64 CSCMatrix::one_norm() const {
    Float64 max_norm = 0.0;
    for (Index j = 0; j < cols_; ++j) {
        Float64 col_sum = 0.0;
        Index start = col_ptr_[j];
        Index end = col_ptr_[j + 1];
        for (Index k = start; k < end; ++k) {
            col_sum += std::abs(values_[k]);
        }
        max_norm = std::max(max_norm, col_sum);
    }
    return max_norm;
}

void CSCMatrix::scale_rows(std::span<const Float64> row_scales) {
    if (row_scales.size() != static_cast<std::size_t>(rows_)) {
        throw std::invalid_argument("Row scales dimension mismatch");
    }
    for (std::size_t k = 0; k < values_.size(); ++k) {
        values_[k] *= row_scales[row_idx_[k]];
    }
}

void CSCMatrix::scale_cols(std::span<const Float64> col_scales) {
    if (col_scales.size() != static_cast<std::size_t>(cols_)) {
        throw std::invalid_argument("Column scales dimension mismatch");
    }
    for (Index j = 0; j < cols_; ++j) {
        Float64 s = col_scales[j];
        Index start = col_ptr_[j];
        Index end = col_ptr_[j + 1];
        for (Index k = start; k < end; ++k) {
            values_[k] *= s;
        }
    }
}

COOMatrix CSCMatrix::to_coo() const {
    COOMatrix coo(rows_, cols_, values_.size());
    for (Index j = 0; j < cols_; ++j) {
        Index start = col_ptr_[j];
        Index end = col_ptr_[j + 1];
        for (Index k = start; k < end; ++k) {
            coo.add_entry(row_idx_[k], j, values_[k]);
        }
    }
    return coo;
}

CSRMatrix CSCMatrix::to_csr() const {
    return to_coo().to_csr();
}

CSCMatrix CSCMatrix::transpose() const {
    return to_coo().to_csr().to_csc(); // Transpose of CSC is CSR with swapped dimensions
}

bool CSCMatrix::is_valid() const noexcept {
    if (rows_ < 0 || cols_ < 0) return false;
    if (col_ptr_.size() != static_cast<std::size_t>(cols_ + 1)) return false;
    if (col_ptr_[0] != 0) return false;
    if (col_ptr_.back() != static_cast<Index>(values_.size())) return false;
    if (row_idx_.size() != values_.size()) return false;

    for (Index j = 0; j < cols_; ++j) {
        if (col_ptr_[j] > col_ptr_[j + 1]) return false;
        Index prev_r = -1;
        for (Index k = col_ptr_[j]; k < col_ptr_[j + 1]; ++k) {
            Index r = row_idx_[k];
            if (r < 0 || r >= rows_) return false;
            if (r <= prev_r) return false; // Must be strictly increasing row indices
            prev_r = r;
        }
    }
    return true;
}

} // namespace sovereign::linalg
