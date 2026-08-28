#ifndef SOVEREIGN_LP_INITIAL_BASIS_HPP
#define SOVEREIGN_LP_INITIAL_BASIS_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/basis.hpp"
#include "sovereign/lp/basis_matrix.hpp"

namespace sovereign::lp {

class InitialBasisBuilder {
public:
    // Constructs a logical initial basis for a standard form LPModel
    static Basis construct_logical_basis(const LPModel& standard_model);

    // Validates whether the initial basis is non-singular and computes starting primal/dual values
    static Status validate_initial_basis(const LPModel& standard_model, const Basis& basis, BasisMatrix& basis_matrix);
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_INITIAL_BASIS_HPP
