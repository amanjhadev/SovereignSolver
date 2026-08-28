#include "sovereign/linalg/csr_matrix.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sovereign::linalg {

CSRMatrix::CSRMatrix(Index rows, Index cols, std::vector<Index> row_ptr, std::vector<Index> col_idx, std::vector<Float64> values)
    : rows_(rows), cols_(cols), row_ptr_(std::move(row_ptr)), col_idx_(std::move(col_idx)), values_(std::move(values)) {
    if (row_ptr_.size() != static_cast<std::size_t>(rows_ + 1)) {
        throw std::invalid_argument("CSR row_ptr size must be rows + 1");
    }
}

std::span<const Index> CSRMatrix::get_row_col_indices(Index row) const {
    if (row < 0 || row >= rows_) {
        throw std::out_of_range("Row index out of bounds");
    }
    Index start = row_ptr_[row];
    Index end = row_ptr_[row + 1];
    return std::span<const Index>(col_idx_.data() + start, end - start);
}

std::span<const Float64> CSRMatrix::get_row_values(Index row) const {
    if (row < 0 || row >= rows_) {
        throw std::out_of_range("Row index out of bounds");
    }
    Index start = row_ptr_[row];
    Index end = row_ptr_[row + 1];
    return std::span<const Float64>(values_.data() + start, end - start);
}

Index CSRMatrix::get_row_nnz(Index row) const {
    if (row < 0 || row >= rows_) {
        throw std::out_of_range("Row index out of bounds");
    }
    return row_ptr_[row + 1] - row_ptr_[row];
}

void CSRMatrix::multiply(std::span<const Float64> x, std::span<Float64> y) const {
    if (x.size() != static_cast<std::size_t>(cols_) || y.size() != static_cast<std::size_t>(rows_)) {
        throw std::invalid_argument("SpMV dimension mismatch in CSR multiply");
    }
    for (Index i = 0; i < rows_; ++i) {
        Float64 sum = 0.0;
        Index start = row_ptr_[i];
        Index end = row_ptr_[i + 1];
        for (Index k = start; k < end; ++k) {
            sum += values_[k] * x[col_idx_[k]];
        }
        y[i] = sum;
    }
}

void CSRMatrix::multiply_transpose(std::span<const Float64> x, std::span<Float64> y) const {
    if (x.size() != static_cast<std::size_t>(rows_) || y.size() != static_cast<std::size_t>(cols_)) {
        throw std::invalid_argument("SpMV dimension mismatch in CSR multiply_transpose");
    }
    std::fill(y.begin(), y.end(), 0.0);
    for (Index i = 0; i < rows_; ++i) {
        Float64 xi = x[i];
        if (xi == 0.0) continue;
        Index start = row_ptr_[i];
        Index end = row_ptr_[i + 1];
        for (Index k = start; k < end; ++k) {
            y[col_idx_[k]] += values_[k] * xi;
        }
    }
}

Float64 CSRMatrix::infinity_norm() const {
    Float64 max_norm = 0.0;
    for (Index i = 0; i < rows_; ++i) {
        Float64 row_sum = 0.0;
        Index start = row_ptr_[i];
        Index end = row_ptr_[i + 1];
        for (Index k = start; k < end; ++k) {
            row_sum += std::abs(values_[k]);
        }
        max_norm = std::max(max_norm, row_sum);
    }
    return max_norm;
}

Float64 CSRMatrix::one_norm() const {
    std::vector<Float64> col_sums(cols_, 0.0);
    for (Index i = 0; i < rows_; ++i) {
        Index start = row_ptr_[i];
        Index end = row_ptr_[i + 1];
        for (Index k = start; k < end; ++k) {
            col_sums[col_idx_[k]] += std::abs(values_[k]);
        }
    }
    Float64 max_norm = 0.0;
    for (Float64 s : col_sums) {
        max_norm = std::max(max_norm, s);
    }
    return max_norm;
}

COOMatrix CSRMatrix::to_coo() const {
    COOMatrix coo(rows_, cols_, values_.size());
    for (Index i = 0; i < rows_; ++i) {
        Index start = row_ptr_[i];
        Index end = row_ptr_[i + 1];
        for (Index k = start; k < end; ++k) {
            coo.add_entry(i, col_idx_[k], values_[k]);
        }
    }
    return coo;
}

CSCMatrix CSRMatrix::to_csc() const {
    return to_coo().to_csc();
}

bool CSRMatrix::is_valid() const noexcept {
    if (rows_ < 0 || cols_ < 0) return false;
    if (row_ptr_.size() != static_cast<std::size_t>(rows_ + 1)) return false;
    if (row_ptr_[0] != 0) return false;
    if (row_ptr_.back() != static_cast<Index>(values_.size())) return false;
    if (col_idx_.size() != values_.size()) return false;

    for (Index i = 0; i < rows_; ++i) {
        if (row_ptr_[i] > row_ptr_[i + 1]) return false;
        Index prev_c = -1;
        for (Index k = row_ptr_[i]; k < row_ptr_[i + 1]; ++k) {
            Index c = col_idx_[k];
            if (c < 0 || c >= cols_) return false;
            if (c <= prev_c) return false; // Strictly increasing col indices
            prev_c = c;
        }
    }
    return true;
}

} // namespace sovereign::linalg
