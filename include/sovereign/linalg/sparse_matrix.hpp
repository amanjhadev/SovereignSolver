#ifndef SOVEREIGN_LINALG_SPARSE_MATRIX_HPP
#define SOVEREIGN_LINALG_SPARSE_MATRIX_HPP

#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/csr_matrix.hpp"
#include <variant>

namespace sovereign::linalg {

enum class MatrixStorageType {
    COO,
    CSC,
    CSR
};

class SparseMatrix {
private:
    std::variant<COOMatrix, CSCMatrix, CSRMatrix> storage_;

public:
    SparseMatrix() = default;
    explicit SparseMatrix(COOMatrix coo) : storage_(std::move(coo)) {}
    explicit SparseMatrix(CSCMatrix csc) : storage_(std::move(csc)) {}
    explicit SparseMatrix(CSRMatrix csr) : storage_(std::move(csr)) {}

    MatrixStorageType storage_type() const noexcept {
        if (std::holds_alternative<COOMatrix>(storage_)) return MatrixStorageType::COO;
        if (std::holds_alternative<CSCMatrix>(storage_)) return MatrixStorageType::CSC;
        return MatrixStorageType::CSR;
    }

    Index num_rows() const noexcept {
        return std::visit([](const auto& m) { return m.num_rows(); }, storage_);
    }

    Index num_cols() const noexcept {
        return std::visit([](const auto& m) { return m.num_cols(); }, storage_);
    }

    std::size_t num_nonzeros() const noexcept {
        return std::visit([](const auto& m) { return m.num_nonzeros(); }, storage_);
    }

    CSCMatrix to_csc() const {
        return std::visit([](const auto& m) -> CSCMatrix {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, CSCMatrix>) {
                return m;
            } else {
                return m.to_csc();
            }
        }, storage_);
    }

    CSRMatrix to_csr() const {
        return std::visit([](const auto& m) -> CSRMatrix {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, CSRMatrix>) {
                return m;
            } else {
                return m.to_csr();
            }
        }, storage_);
    }
};

} // namespace sovereign::linalg

#endif // SOVEREIGN_LINALG_SPARSE_MATRIX_HPP
