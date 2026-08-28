#include "sovereign/milp/mir_cut.hpp"
#include <cmath>
#include <algorithm>

namespace sovereign::milp {

CanonicalMIRCutGenerator::CanonicalMIRCutGenerator(Float64 tol_integrality, Float64 tol_violation)
    : tol_integrality_(tol_integrality), tol_violation_(tol_violation) {}

std::optional<Cut> CanonicalMIRCutGenerator::separate_row(
    const MILPModel& model,
    std::span<const Index> row_cols,
    std::span<const Float64> row_vals,
    Float64 rhs,
    std::span<const Float64> current_sol,
    Int64 origin_node_id
) const {
    if (row_cols.empty() || row_cols.size() != row_vals.size()) {
        return std::nullopt; // Precondition failure: invalid row
    }

    Float64 f0 = rhs - std::floor(rhs);
    if (f0 <= tol_integrality_ || f0 >= 1.0 - tol_integrality_) {
        return std::nullopt; // Precondition failure: RHS is integral
    }

    // Partition variables into integer set I and continuous set C
    std::vector<Index> int_cols;
    std::vector<Float64> int_vals;

    std::vector<Index> cont_cols;
    std::vector<Float64> cont_vals;

    for (std::size_t k = 0; k < row_cols.size(); ++k) {
        Index col = row_cols[k];
        if (col < 0 || col >= model.num_variables()) continue;

        if (model.is_integer(col)) {
            int_cols.push_back(col);
            int_vals.push_back(row_vals[k]);
        } else {
            cont_cols.push_back(col);
            cont_vals.push_back(row_vals[k]);
        }
    }

    if (int_cols.empty()) {
        return std::nullopt; // Precondition failure: no integer variables
    }

    // Compute canonical MIR inequality coefficients:
    // \sum_{j \in I} [ floor(a_j) + max(0, f(a_j) - f0) / (1 - f0) ] y_j <= floor(b) + x / (1 - f0)
    std::vector<Index> cut_cols;
    std::vector<Float64> cut_coeffs;

    Float64 mir_rhs = std::floor(rhs);

    for (std::size_t k = 0; k < int_cols.size(); ++k) {
        Index col = int_cols[k];
        Float64 a_j = int_vals[k];
        Float64 floor_a = std::floor(a_j);
        Float64 f_a = a_j - floor_a;

        Float64 mir_coeff = floor_a;
        if (f_a > f0) {
            mir_coeff += (f_a - f0) / (1.0 - f0);
        }

        cut_cols.push_back(col);
        cut_coeffs.push_back(mir_coeff);
    }

    for (std::size_t k = 0; k < cont_cols.size(); ++k) {
        Index col = cont_cols[k];
        Float64 a_j = cont_vals[k];
        if (a_j > 0.0) {
            cut_cols.push_back(col);
            cut_coeffs.push_back(-a_j / (1.0 - f0));
        } else if (a_j < 0.0) {
            cut_cols.push_back(col);
            cut_coeffs.push_back(a_j / (1.0 - f0));
        }
    }

    // Post-translation validity & solution violation check
    Float64 lhs_val = 0.0;
    for (std::size_t k = 0; k < cut_cols.size(); ++k) {
        Index col = cut_cols[k];
        if (col >= 0 && col < static_cast<Index>(current_sol.size())) {
            lhs_val += cut_coeffs[k] * current_sol[col];
        }
    }

    Float64 viol = lhs_val - mir_rhs;
    if (viol < tol_violation_) {
        return std::nullopt; // Fails violation check
    }

    Cut cut;
    cut.name = "canonical_mir_cut_n" + std::to_string(origin_node_id);
    cut.sense = lp::ConstraintSense::LessEqual;
    cut.rhs = mir_rhs;
    cut.col_indices = std::move(cut_cols);
    cut.coefficients = std::move(cut_coeffs);
    cut.scope = (origin_node_id <= 0) ? CutScope::Global : CutScope::LocalSubtree;
    cut.origin_node_id = origin_node_id;
    cut.type = CutType::CanonicalMIR;

    return cut;
}

std::vector<Cut> CanonicalMIRCutGenerator::generate_cuts(
    const MILPModel& model,
    const lp::LPModel& current_lp,
    const lp::SimplexState& state,
    Int64 origin_node_id
) const {
    std::vector<Cut> generated_cuts;

    const auto& csc = current_lp.get_csc_matrix();
    Index m = current_lp.num_constraints();
    Index n = current_lp.num_variables();
    const auto& x_all = state.x_all();

    for (Index i = 0; i < m; ++i) {
        const auto& c = current_lp.get_constraint(i);
        if (c.sense != lp::ConstraintSense::LessEqual && c.sense != lp::ConstraintSense::Equal) continue;

        std::vector<Index> row_cols;
        std::vector<Float64> row_vals;

        for (Index j = 0; j < n; ++j) {
            auto r_idx = csc.get_col_row_indices(j);
            auto c_v = csc.get_col_values(j);
            for (std::size_t k = 0; k < r_idx.size(); ++k) {
                if (r_idx[k] == i) {
                    row_cols.push_back(j);
                    row_vals.push_back(c_v[k]);
                }
            }
        }

        auto cut_opt = separate_row(model, row_cols, row_vals, c.rhs, x_all, origin_node_id);
        if (cut_opt.has_value()) {
            generated_cuts.push_back(std::move(*cut_opt));
        }
    }

    return generated_cuts;
}

} // namespace sovereign::milp
