#include "sovereign/lp/initial_basis.hpp"
#include <stdexcept>
#include <iostream>

namespace sovereign::lp {

Basis InitialBasisBuilder::construct_logical_basis(const LPModel& standard_model) {
    Index m = standard_model.num_constraints();
    Index n = standard_model.num_variables();

    if (m == 0 || n < m) {
        throw std::invalid_argument("Invalid model dimensions for initial basis construction");
    }

    Basis basis(m, n);
    std::vector<bool> used_as_basic(n, false);

    // 1. Identify dedicated slack/surplus/range columns for each row
    std::vector<Index> row_slack_col(m, -1);
    for (Index j = 0; j < n; ++j) {
        const auto& var = standard_model.get_variable(j);
        if (var.constraint_index >= 0 && var.constraint_index < m) {
            row_slack_col[var.constraint_index] = j;
        }
    }

    // 2. Assign basic variables slot by slot
    const auto& csc = standard_model.get_csc_matrix();
    for (Index i = 0; i < m; ++i) {
        Index slack_col = row_slack_col[i];
        if (slack_col >= 0 && slack_col < n && !used_as_basic[slack_col]) {
            basis.set_basic(slack_col, i);
            used_as_basic[slack_col] = true;
        } else {
            // For equality rows without slacks, search for candidate structural column with non-zero entry in row i
            Index candidate_col = -1;
            for (Index j = 0; j < n; ++j) {
                if (used_as_basic[j]) continue;
                auto r_indices = csc.get_col_row_indices(j);
                auto c_vals = csc.get_col_values(j);
                for (std::size_t k = 0; k < r_indices.size(); ++k) {
                    if (r_indices[k] == i && std::abs(c_vals[k]) > ZERO_TOLERANCE) {
                        candidate_col = j;
                        break;
                    }
                }
                if (candidate_col != -1) break;
            }

            if (candidate_col != -1) {
                basis.set_basic(candidate_col, i);
                used_as_basic[candidate_col] = true;
            }
        }
    }

    // 3. Set non-basic status for remaining columns
    for (Index j = 0; j < n; ++j) {
        if (basis.get_status(j) != BasisStatus::Basic) {
            const auto& var = standard_model.get_variable(j);
            if (var.lower_bound > -INFINITY_VAL) {
                basis.set_nonbasic(j, BasisStatus::AtLower);
            } else if (var.upper_bound < INFINITY_VAL) {
                basis.set_nonbasic(j, BasisStatus::AtUpper);
            } else {
                basis.set_nonbasic(j, BasisStatus::Free);
            }
        }
    }

    basis.rebuild_position_maps();
    return basis;
}

Status InitialBasisBuilder::validate_initial_basis(const LPModel& standard_model, const Basis& basis, BasisMatrix& basis_matrix) {
    if (!basis.is_valid()) {
        return Status::InvalidInput;
    }

    const auto& csc = standard_model.get_csc_matrix();
    Status status = basis_matrix.factorize(csc, basis);
    return status;
}

} // namespace sovereign::lp
