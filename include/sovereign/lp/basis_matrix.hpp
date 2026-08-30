#ifndef SOVEREIGN_LP_BASIS_MATRIX_HPP
#define SOVEREIGN_LP_BASIS_MATRIX_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/lu_factor.hpp"
#include "sovereign/lp/basis.hpp"
#include <span>
#include <vector>

namespace sovereign::lp {

struct EtaUpdate {
    Index slot_p{0};
    Float64 alpha_p{1.0};
    std::vector<Index> nz_rows;
    std::vector<Float64> nz_vals;
};

class BasisMatrix {
private:
    Index m_{0};
    linalg::CSCMatrix B_csc_;
    linalg::SparseLU lu_;
    std::vector<EtaUpdate> updates_;
    Int32 max_updates_{50};
    bool is_factored_{false};

public:
    BasisMatrix() = default;

    // Gathers columns from A corresponding to basic_vars in basis, and factorizes B
    Status factorize(const linalg::CSCMatrix& A, const Basis& basis, const SolverConfig& config = SolverConfig::default_config());

    // Product-Form Eta Basis Update
    Status update_pivot(Index slot_p, std::span<const Float64> ftran_d, Float64 pivot_tol = 1e-8);

    // Solves B * x = b (Primal basic solve / FTRAN)
    Status solve_primal(std::span<const Float64> b, std::span<Float64> x) const;

    // Solves B^T * y = c_B (Dual basic solve / BTRAN)
    Status solve_dual(std::span<const Float64> c_B, std::span<Float64> y) const;

    // Extracts column of B at basis slot j
    void extract_basis_column(const linalg::CSCMatrix& A, const Basis& basis, Index slot, std::span<Float64> col_out) const;

    bool is_factored() const noexcept { return is_factored_; }
    bool should_refactorize() const noexcept {
        return !is_factored_ || static_cast<Int32>(updates_.size()) >= max_updates_;
    }
    Int32 update_count() const noexcept { return static_cast<Int32>(updates_.size()); }

    Status status() const noexcept { return lu_.status(); }
    const linalg::SparseLU& lu() const noexcept { return lu_; }
    const linalg::CSCMatrix& B() const noexcept { return B_csc_; }
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_BASIS_MATRIX_HPP
