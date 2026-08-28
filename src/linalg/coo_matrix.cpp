#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/csr_matrix.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace sovereign::linalg {

COOMatrix::COOMatrix(Index rows, Index cols, std::size_t initial_capacity)
    : rows_(rows), cols_(cols) {
    if (initial_capacity > 0) {
        reserve(initial_capacity);
    }
}

void COOMatrix::resize_dimensions(Index rows, Index cols) noexcept {
    rows_ = std::max(rows_, rows);
    cols_ = std::max(cols_, cols);
}

void COOMatrix::add_entry(Index row, Index col, Float64 value) {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        throw std::out_of_range("COO entry row or column out of bounds");
    }
    row_indices_.push_back(row);
    col_indices_.push_back(col);
    values_.push_back(value);
}

void COOMatrix::clear() noexcept {
    row_indices_.clear();
    col_indices_.clear();
    values_.clear();
}

void COOMatrix::reserve(std::size_t capacity) {
    row_indices_.reserve(capacity);
    col_indices_.reserve(capacity);
    values_.reserve(capacity);
}

CSCMatrix COOMatrix::to_csc() const {
    std::size_t nnz = values_.size();
    if (nnz == 0) {
        return CSCMatrix(rows_, cols_, std::vector<Index>(cols_ + 1, 0), {}, {});
    }

    // Permutation vector for sorting by (col, row)
    std::vector<std::size_t> p(nnz);
    std::iota(p.begin(), p.end(), 0);

    std::stable_sort(p.begin(), p.end(), [this](std::size_t i, std::size_t j) {
        if (col_indices_[i] != col_indices_[j]) {
            return col_indices_[i] < col_indices_[j];
        }
        return row_indices_[i] < row_indices_[j];
    });

    std::vector<Index> col_ptr(cols_ + 1, 0);
    std::vector<Index> csc_row_idx;
    std::vector<Float64> csc_values;
    csc_row_idx.reserve(nnz);
    csc_values.reserve(nnz);

    Index prev_row = -1;
    Index prev_col = -1;

    for (std::size_t idx : p) {
        Index r = row_indices_[idx];
        Index c = col_indices_[idx];
        Float64 val = values_[idx];

        if (r == prev_row && c == prev_col) {
            // Sum duplicate entry
            csc_values.back() += val;
        } else {
            csc_row_idx.push_back(r);
            csc_values.push_back(val);
            col_ptr[c + 1]++;
            prev_row = r;
            prev_col = c;
        }
    }

    // Prefix sum for column pointers
    for (Index j = 0; j < cols_; ++j) {
        col_ptr[j + 1] += col_ptr[j];
    }

    return CSCMatrix(rows_, cols_, std::move(col_ptr), std::move(csc_row_idx), std::move(csc_values));
}

CSRMatrix COOMatrix::to_csr() const {
    std::size_t nnz = values_.size();
    if (nnz == 0) {
        return CSRMatrix(rows_, cols_, std::vector<Index>(rows_ + 1, 0), {}, {});
    }

    std::vector<std::size_t> p(nnz);
    std::iota(p.begin(), p.end(), 0);

    std::stable_sort(p.begin(), p.end(), [this](std::size_t i, std::size_t j) {
        if (row_indices_[i] != row_indices_[j]) {
            return row_indices_[i] < row_indices_[j];
        }
        return col_indices_[i] < col_indices_[j];
    });

    std::vector<Index> row_ptr(rows_ + 1, 0);
    std::vector<Index> csr_col_idx;
    std::vector<Float64> csr_values;
    csr_col_idx.reserve(nnz);
    csr_values.reserve(nnz);

    Index prev_row = -1;
    Index prev_col = -1;

    for (std::size_t idx : p) {
        Index r = row_indices_[idx];
        Index c = col_indices_[idx];
        Float64 val = values_[idx];

        if (r == prev_row && c == prev_col) {
            csr_values.back() += val;
        } else {
            csr_col_idx.push_back(c);
            csr_values.push_back(val);
            row_ptr[r + 1]++;
            prev_row = r;
            prev_col = c;
        }
    }

    for (Index i = 0; i < rows_; ++i) {
        row_ptr[i + 1] += row_ptr[i];
    }

    return CSRMatrix(rows_, cols_, std::move(row_ptr), std::move(csr_col_idx), std::move(csr_values));
}

} // namespace sovereign::linalg
