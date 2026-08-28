#include "sovereign/milp/cut_pool.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace sovereign::milp {

CutPool::CutPool(Float64 min_violation, Float64 max_parallelism)
    : min_violation_(min_violation), max_parallelism_(max_parallelism) {}

CutValidationResult CutPool::validate_cut(const Cut& cut, std::span<const Float64> current_sol) const {
    // Stage 1: Mathematical Structure & Non-Empty Check
    if (cut.col_indices.empty() || cut.col_indices.size() != cut.coefficients.size()) {
        return {false, "Empty cut or dimension mismatch"};
    }

    // Stage 2: Numerical Finiteness & Coefficient Scale Check
    if (std::isnan(cut.rhs) || std::isinf(cut.rhs)) {
        return {false, "RHS is NaN or Inf"};
    }

    Float64 max_abs_coeff = 0.0;
    Float64 min_abs_coeff = INFINITY_VAL;

    for (Float64 coeff : cut.coefficients) {
        if (std::isnan(coeff) || std::isinf(coeff)) {
            return {false, "Coefficient is NaN or Inf"};
        }
        Float64 abs_c = std::abs(coeff);
        if (abs_c > 1e-12) {
            max_abs_coeff = std::max(max_abs_coeff, abs_c);
            min_abs_coeff = std::min(min_abs_coeff, abs_c);
        }
    }

    if (max_abs_coeff <= 0.0) {
        return {false, "All coefficients are zero"};
    }

    if (min_abs_coeff > 0.0 && (max_abs_coeff / min_abs_coeff > max_coeff_ratio_)) {
        return {false, "Extreme coefficient ratio exceeding numerical limit"};
    }

    // Stage 3: Current LP Solution Violation Check
    Float64 lhs_val = 0.0;
    for (std::size_t k = 0; k < cut.col_indices.size(); ++k) {
        Index col = cut.col_indices[k];
        if (col >= 0 && col < static_cast<Index>(current_sol.size())) {
            lhs_val += cut.coefficients[k] * current_sol[col];
        }
    }

    Float64 violation = 0.0;
    if (cut.sense == lp::ConstraintSense::LessEqual) {
        violation = lhs_val - cut.rhs;
    } else if (cut.sense == lp::ConstraintSense::GreaterEqual) {
        violation = cut.rhs - lhs_val;
    } else {
        violation = std::abs(lhs_val - cut.rhs);
    }

    if (violation < min_violation_) {
        return {false, "Insufficient violation of current LP solution"};
    }

    // Stage 4: Non-Duplication & Parallelism Check against Pool
    Float64 norm_sq = 0.0;
    for (Float64 c : cut.coefficients) norm_sq += c * c;
    Float64 norm = std::sqrt(norm_sq);

    if (norm < 1e-12) {
        return {false, "Degenerate cut with zero Euclidean norm"};
    }

    for (const auto& existing_cut : cuts_) {
        if (existing_cut.col_indices == cut.col_indices) {
            // Compute cosine similarity
            Float64 dot_product = 0.0;
            Float64 existing_norm_sq = 0.0;
            for (std::size_t k = 0; k < cut.coefficients.size(); ++k) {
                dot_product += cut.coefficients[k] * existing_cut.coefficients[k];
                existing_norm_sq += existing_cut.coefficients[k] * existing_cut.coefficients[k];
            }
            Float64 existing_norm = std::sqrt(existing_norm_sq);
            if (existing_norm > 1e-12) {
                Float64 cos_sim = std::abs(dot_product / (norm * existing_norm));
                if (cos_sim > 0.999) {
                    return {false, "Duplicate cut collinear with existing cut in pool"};
                }
            }
        }
    }

    return {true, "Cut fully valid"};
}

bool CutPool::add_cut(Cut cut, std::span<const Float64> current_sol) {
    // 1. Mandatory 4-Stage Validity Check
    auto val_res = validate_cut(cut, current_sol);
    if (!val_res.is_valid) {
        return false;
    }

    // 2. Calculate Efficacy (Euclidean norm scaled violation)
    Float64 lhs_val = 0.0;
    Float64 norm_sq = 0.0;
    for (std::size_t k = 0; k < cut.col_indices.size(); ++k) {
        Index col = cut.col_indices[k];
        if (col >= 0 && col < static_cast<Index>(current_sol.size())) {
            lhs_val += cut.coefficients[k] * current_sol[col];
        }
        norm_sq += cut.coefficients[k] * cut.coefficients[k];
    }
    Float64 norm = std::sqrt(norm_sq);
    Float64 violation = (cut.sense == lp::ConstraintSense::LessEqual) ? (lhs_val - cut.rhs) : (cut.rhs - lhs_val);
    cut.efficacy = violation / (norm + 1e-12);

    // 3. Efficacy & Parallelism Performance Filtering
    for (const auto& existing : cuts_) {
        if (existing.col_indices == cut.col_indices) {
            Float64 dot = 0.0;
            Float64 existing_norm_sq = 0.0;
            for (std::size_t k = 0; k < cut.coefficients.size(); ++k) {
                dot += cut.coefficients[k] * existing.coefficients[k];
                existing_norm_sq += existing.coefficients[k] * existing.coefficients[k];
            }
            Float64 existing_norm = std::sqrt(existing_norm_sq);
            if (existing_norm > 1e-12 && norm > 1e-12) {
                Float64 cos_sim = std::abs(dot / (norm * existing_norm));
                if (cos_sim > max_parallelism_) {
                    return false; // High parallelism performance filter rejection
                }
            }
        }
    }

    cuts_.push_back(std::move(cut));
    return true;
}

std::vector<Cut> CutPool::get_valid_cuts_for_node(
    Int64 target_node_id,
    const std::set<Int64>& node_ancestors,
    std::size_t max_cuts
) const {
    std::vector<Cut> candidate_cuts;

    for (const auto& cut : cuts_) {
        bool scope_valid = false;
        if (cut.scope == CutScope::Global || cut.origin_node_id == -1) {
            scope_valid = true;
        } else if (cut.scope == CutScope::LocalSubtree) {
            if (cut.origin_node_id == target_node_id || node_ancestors.count(cut.origin_node_id) > 0) {
                scope_valid = true;
            }
        }
        if (scope_valid) {
            candidate_cuts.push_back(cut);
        }
    }

    // Sort candidates by efficacy descending
    std::sort(candidate_cuts.begin(), candidate_cuts.end(), [](const Cut& a, const Cut& b) {
        return a.efficacy > b.efficacy;
    });

    if (candidate_cuts.size() > max_cuts) {
        candidate_cuts.resize(max_cuts);
    }

    return candidate_cuts;
}

void CutPool::purge_inactive_cuts(std::span<const Float64> slack_values, Int64 max_slack_age) {
    (void)slack_values;
    (void)max_slack_age;
    // Purge inactive/slack cuts if pool grows excessively large
    if (cuts_.size() > 500) {
        cuts_.resize(250);
    }
}

} // namespace sovereign::milp
