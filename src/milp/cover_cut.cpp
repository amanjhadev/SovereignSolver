#include "sovereign/milp/cover_cut.hpp"
#include <cmath>
#include <algorithm>

namespace sovereign::milp {

MinimalCoverCutGenerator::MinimalCoverCutGenerator(Float64 tol_violation)
    : tol_violation_(tol_violation) {}

std::optional<Cut> MinimalCoverCutGenerator::separate_knapsack_row(
    const MILPModel& model,
    std::span<const Index> row_cols,
    std::span<const Float64> row_vals,
    Float64 rhs,
    std::span<const Float64> current_sol,
    Int64 origin_node_id
) const {
    if (row_cols.empty() || row_cols.size() != row_vals.size()) return std::nullopt;

    // Filter to positive binary variables: a_j > 0, x_j in {0, 1}
    struct KnapsackItem {
        Index col;
        Float64 weight;
        Float64 sol_val;
    };

    std::vector<KnapsackItem> items;
    for (std::size_t k = 0; k < row_cols.size(); ++k) {
        Index col = row_cols[k];
        if (col < 0 || col >= model.num_variables()) continue;

        if (model.is_binary(col) && row_vals[k] > 1e-9) {
            Float64 x_val = (col < static_cast<Index>(current_sol.size())) ? current_sol[col] : 0.0;
            items.push_back({col, row_vals[k], x_val});
        }
    }

    if (items.size() < 2) return std::nullopt;

    // Sort items by current solution value descending (greedy cover search)
    std::sort(items.begin(), items.end(), [](const KnapsackItem& a, const KnapsackItem& b) {
        return a.sol_val > b.sol_val;
    });

    std::vector<Index> cover_cols;
    Float64 weight_sum = 0.0;

    for (const auto& item : items) {
        if (item.sol_val > 1e-4) {
            cover_cols.push_back(item.col);
            weight_sum += item.weight;
            if (weight_sum > rhs + 1e-6) {
                break;
            }
        }
    }

    if (weight_sum <= rhs + 1e-6) return std::nullopt; // No cover found

    // Minimize cover by removing redundant items
    for (auto it = cover_cols.begin(); it != cover_cols.end(); ) {
        Index col = *it;
        Float64 item_w = 0.0;
        for (const auto& item : items) {
            if (item.col == col) {
                item_w = item.weight;
                break;
            }
        }
        if (weight_sum - item_w > rhs + 1e-6) {
            weight_sum -= item_w;
            it = cover_cols.erase(it);
        } else {
            ++it;
        }
    }

    if (cover_cols.size() < 2) return std::nullopt;

    // Minimal cover inequality: \sum_{j \in C} x_j <= |C| - 1
    Float64 cover_rhs = static_cast<Float64>(cover_cols.size() - 1);
    std::vector<Float64> cover_coeffs(cover_cols.size(), 1.0);

    // Violation check: \sum_{j \in C} x_j^* > |C| - 1
    Float64 lhs_val = 0.0;
    for (Index col : cover_cols) {
        if (col >= 0 && col < static_cast<Index>(current_sol.size())) {
            lhs_val += current_sol[col];
        }
    }

    Float64 viol = lhs_val - cover_rhs;
    if (viol < tol_violation_) {
        return std::nullopt; // Fails violation check
    }

    Cut cut;
    cut.name = "minimal_cover_cut_n" + std::to_string(origin_node_id);
    cut.sense = lp::ConstraintSense::LessEqual;
    cut.rhs = cover_rhs;
    cut.col_indices = std::move(cover_cols);
    cut.coefficients = std::move(cover_coeffs);
    cut.scope = (origin_node_id <= 0) ? CutScope::Global : CutScope::LocalSubtree;
    cut.origin_node_id = origin_node_id;
    cut.type = CutType::MinimalKnapsackCover;

    return cut;
}

std::vector<Cut> MinimalCoverCutGenerator::generate_cuts(
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
        if (c.sense != lp::ConstraintSense::LessEqual) continue;

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

        auto cut_opt = separate_knapsack_row(model, row_cols, row_vals, c.rhs, x_all, origin_node_id);
        if (cut_opt.has_value()) {
            generated_cuts.push_back(std::move(*cut_opt));
        }
    }

    return generated_cuts;
}

} // namespace sovereign::milp
