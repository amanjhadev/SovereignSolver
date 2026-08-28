#include "sovereign/milp/gomory_cut.hpp"
#include <cmath>
#include <algorithm>
#include <map>

namespace sovereign::milp {

GomoryCutGenerator::GomoryCutGenerator(Float64 tol_integrality, Float64 tol_violation)
    : tol_integrality_(tol_integrality), tol_violation_(tol_violation) {}

std::vector<Cut> GomoryCutGenerator::generate_cuts(
    const MILPModel& model,
    const lp::LPModel& current_lp,
    const lp::SimplexState& state,
    Int64 origin_node_id
) const {
    (void)current_lp;
    std::vector<Cut> generated_cuts;

    Index m = state.num_rows();
    Index n = state.num_cols();
    if (m <= 0 || n <= 0) return generated_cuts;

    const auto& std_model = state.std_model();
    const auto& basis = state.basis();
    const auto& x_B = state.x_B();
    const auto& x_all = state.x_all();

    // Iterate through basic variables in current optimal basis
    for (Index i = 0; i < m; ++i) {
        Index basic_col = basis.get_basic_var(i);
        if (basic_col < 0 || basic_col >= n) continue;

        const auto& basic_var = std_model.get_variable(basic_col);
        Index orig_idx = basic_var.original_var_index;

        // Stage 1: Precondition check — Basic variable MUST be an INTEGER variable with fractional value
        if (orig_idx < 0 || orig_idx >= model.num_variables() || !model.is_integer(orig_idx)) continue;

        Float64 b_bar = x_B[i];
        Float64 f0 = b_bar - std::floor(b_bar);
        if (f0 <= tol_integrality_ || f0 >= 1.0 - tol_integrality_) continue;

        // Stage 2: Compute BTRAN row e_i^T B^-1 to get tableau coefficients alpha_ij = e_i^T B^-1 A_j
        std::vector<Float64> y_p(m, 0.0);
        if (!is_ok(state.compute_btran_row(i, y_p))) continue;

        std::vector<Float64> alpha(n, 0.0);
        state.compute_row_alphas(y_p, alpha);

        // Stage 3: Construct Gomory fractional inequality on structural non-basic variables
        // \sum_{j \in N_struct} f_j x_j >= f0
        std::map<Index, Float64> struct_coeffs;
        Float64 final_rhs = f0;

        for (Index j = 0; j < n; ++j) {
            if (basis.get_status(j) == lp::BasisStatus::Basic) continue;

            Float64 a_ij = alpha[j];
            Float64 f_j = a_ij - std::floor(a_ij);
            if (f_j <= 1e-9) continue;

            const auto& var_j = std_model.get_variable(j);
            Index orig_j = var_j.original_var_index;

            if (orig_j >= 0 && orig_j < model.num_variables()) {
                // Structural non-basic variable
                struct_coeffs[orig_j] += f_j;
            }
        }

        if (struct_coeffs.empty()) continue;

        std::vector<Index> cut_cols;
        std::vector<Float64> cut_coeffs;
        for (const auto& [col, coeff] : struct_coeffs) {
            if (std::abs(coeff) > 1e-9) {
                cut_cols.push_back(col);
                cut_coeffs.push_back(-coeff); // Convert \sum f_j x_j >= f0 to LessEqual: \sum (-f_j) x_j <= -f0
            }
        }

        if (cut_cols.empty()) continue;

        Cut cut;
        cut.name = "gomory_cut_row" + std::to_string(i) + "_n" + std::to_string(origin_node_id);
        cut.sense = lp::ConstraintSense::LessEqual;
        cut.rhs = -final_rhs;
        cut.col_indices = std::move(cut_cols);
        cut.coefficients = std::move(cut_coeffs);
        cut.scope = (origin_node_id <= 0) ? CutScope::Global : CutScope::LocalSubtree;
        cut.origin_node_id = origin_node_id;
        cut.type = CutType::GomoryFractional;

        // Stage 4: Post-Translation Validity Check
        Float64 lhs_val = 0.0;
        bool numerical_ok = true;
        for (std::size_t k = 0; k < cut.col_indices.size(); ++k) {
            Index c = cut.col_indices[k];
            if (c >= 0 && c < static_cast<Index>(x_all.size())) {
                lhs_val += cut.coefficients[k] * x_all[c];
            }
            if (std::isnan(cut.coefficients[k]) || std::isinf(cut.coefficients[k])) {
                numerical_ok = false;
                break;
            }
        }

        if (!numerical_ok) continue;

        Float64 viol = lhs_val - cut.rhs;
        if (viol >= tol_violation_) {
            generated_cuts.push_back(std::move(cut));
        }
    }

    return generated_cuts;
}

} // namespace sovereign::milp
