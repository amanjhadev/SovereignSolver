#include "sovereign/lp/ratio_test.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace sovereign::lp {

DualRatioTest::DualRatioTest(Float64 tol_primal, Float64 tol_dual, Float64 tol_pivot, Float64 tol_harris)
    : tol_primal_(tol_primal), tol_dual_(tol_dual), tol_pivot_(tol_pivot), tol_harris_(tol_harris) {}

RatioTestResult DualRatioTest::select_entering_variable(
    const SimplexState& state,
    Index leaving_slot,
    std::span<const Float64> row_alphas
) const {
    RatioTestResult result;
    if (leaving_slot < 0 || leaving_slot >= state.num_rows()) {
        return result;
    }

    Index leaving_col = state.basis().get_basic_var(leaving_slot);
    const auto& var_l = state.std_model().get_variable(leaving_col);
    Float64 val_l = state.x_B()[leaving_slot];

    bool lower_viol = (val_l < var_l.lower_bound - tol_primal_);
    bool upper_viol = (val_l > var_l.upper_bound + tol_primal_);

    if (!lower_viol && !upper_viol) {
        return result; // No bound violation at this row
    }

    const auto& rc = state.reduced_costs();
    Float64 min_ratio = INFINITY_VAL;
    Index best_col = -1;

    for (Index j = 0; j < state.num_cols(); ++j) {
        BasisStatus st = state.basis().get_status(j);
        if (st == BasisStatus::Basic) continue;

        Float64 alpha = row_alphas[j];
        Float64 c_bar = rc[j];
        Float64 ratio = -1.0;
        bool valid_cand = false;

        if (lower_viol) {
            if ((st == BasisStatus::AtLower || st == BasisStatus::Fixed) && alpha < -tol_pivot_) {
                Float64 num = std::max(0.0, c_bar);
                ratio = num / (-alpha);
                valid_cand = true;
            } else if (st == BasisStatus::AtUpper && alpha > tol_pivot_) {
                Float64 num = std::max(0.0, -c_bar);
                ratio = num / alpha;
                valid_cand = true;
            }
        } else if (upper_viol) {
            if ((st == BasisStatus::AtLower || st == BasisStatus::Fixed) && alpha > tol_pivot_) {
                Float64 num = std::max(0.0, c_bar);
                ratio = num / alpha;
                valid_cand = true;
            } else if (st == BasisStatus::AtUpper && alpha < -tol_pivot_) {
                Float64 num = std::max(0.0, -c_bar);
                ratio = num / (-alpha);
                valid_cand = true;
            }
        }

        if (valid_cand && ratio < min_ratio) {
            min_ratio = ratio;
            best_col = j;
        }
    }

    if (best_col != -1) {
        result.entering_col = best_col;
        result.step_length = min_ratio;
        result.valid = true;
    }

    return result;
}

RatioTestResult DualRatioTest::select_entering_variable_harris(
    const SimplexState& state,
    Index leaving_slot,
    std::span<const Float64> row_alphas
) const {
    RatioTestResult result;
    if (leaving_slot < 0 || leaving_slot >= state.num_rows()) {
        return result;
    }

    Index leaving_col = state.basis().get_basic_var(leaving_slot);
    const auto& var_l = state.std_model().get_variable(leaving_col);
    Float64 val_l = state.x_B()[leaving_slot];

    bool lower_viol = (val_l < var_l.lower_bound - tol_primal_);
    bool upper_viol = (val_l > var_l.upper_bound + tol_primal_);

    if (!lower_viol && !upper_viol) {
        return result;
    }

    const auto& rc = state.reduced_costs();
    struct Candidate {
        Index col;
        Float64 ratio;
        Float64 abs_alpha;
    };
    std::vector<Candidate> candidates;
    Float64 min_ratio = INFINITY_VAL;

    for (Index j = 0; j < state.num_cols(); ++j) {
        BasisStatus st = state.basis().get_status(j);
        if (st == BasisStatus::Basic) continue;

        Float64 alpha = row_alphas[j];
        Float64 c_bar = rc[j];
        Float64 ratio = -1.0;
        bool valid_cand = false;

        if (lower_viol) {
            if ((st == BasisStatus::AtLower || st == BasisStatus::Fixed) && alpha < -tol_pivot_) {
                Float64 num = std::max(0.0, c_bar);
                ratio = num / (-alpha);
                valid_cand = true;
            } else if (st == BasisStatus::AtUpper && alpha > tol_pivot_) {
                Float64 num = std::max(0.0, -c_bar);
                ratio = num / alpha;
                valid_cand = true;
            }
        } else if (upper_viol) {
            if ((st == BasisStatus::AtLower || st == BasisStatus::Fixed) && alpha > tol_pivot_) {
                Float64 num = std::max(0.0, c_bar);
                ratio = num / alpha;
                valid_cand = true;
            } else if (st == BasisStatus::AtUpper && alpha < -tol_pivot_) {
                Float64 num = std::max(0.0, -c_bar);
                ratio = num / (-alpha);
                valid_cand = true;
            }
        }

        if (valid_cand) {
            candidates.push_back({j, ratio, std::abs(alpha)});
            if (ratio < min_ratio) {
                min_ratio = ratio;
            }
        }
    }

    if (candidates.empty()) {
        return result;
    }

    // Harris Pass 2: Select candidate within min_ratio + tol_harris with maximum pivot magnitude |alpha|
    Index best_col = -1;
    Float64 max_alpha = -1.0;
    Float64 selected_ratio = min_ratio;

    for (const auto& cand : candidates) {
        if (cand.ratio <= min_ratio + tol_harris_) {
            if (cand.abs_alpha > max_alpha) {
                max_alpha = cand.abs_alpha;
                best_col = cand.col;
                selected_ratio = cand.ratio;
            }
        }
    }

    if (best_col != -1) {
        result.entering_col = best_col;
        result.step_length = selected_ratio;
        result.valid = true;
    }

    return result;
}

} // namespace sovereign::lp
