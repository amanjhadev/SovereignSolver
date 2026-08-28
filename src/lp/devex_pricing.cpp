#include "sovereign/lp/devex_pricing.hpp"
#include <cmath>
#include <algorithm>

namespace sovereign::lp {

DevexPricing::DevexPricing(Index num_rows)
    : m_(num_rows), weights_(num_rows, 1.0) {}

void DevexPricing::reset_weights() {
    weights_.assign(m_, 1.0);
    pivot_count_ = 0;
}

Index DevexPricing::select_leaving_variable(const SimplexState& state, Float64 tol_primal, Float64& max_weighted_viol) const {
    max_weighted_viol = 0.0;
    Index worst_slot = -1;

    for (Index i = 0; i < state.num_rows(); ++i) {
        Index col = state.basis().get_basic_var(i);
        const auto& var = state.std_model().get_variable(col);
        Float64 val = state.x_B()[i];

        Float64 viol_lb = var.lower_bound - val;
        Float64 viol_ub = val - var.upper_bound;
        Float64 viol = std::max(0.0, std::max(viol_lb, viol_ub));

        if (viol > tol_primal) {
            Float64 w = (i < static_cast<Index>(weights_.size())) ? weights_[i] : 1.0;
            Float64 weighted_viol = viol / std::max(1e-4, w);
            if (weighted_viol > max_weighted_viol) {
                max_weighted_viol = weighted_viol;
                worst_slot = i;
            }
        }
    }

    return worst_slot;
}

void DevexPricing::update_weights(Index leaving_slot, std::span<const Float64> d_direction) {
    if (leaving_slot < 0 || leaving_slot >= m_ || d_direction.size() != static_cast<std::size_t>(m_)) {
        return;
    }

    Float64 pivot_d = std::abs(d_direction[leaving_slot]);
    if (pivot_d < 1e-9) pivot_d = 1e-9;

    Float64 gamma_p = weights_[leaving_slot];
    Float64 new_gamma_p = std::max(1.0, gamma_p / pivot_d);

    for (Index i = 0; i < m_; ++i) {
        if (i == leaving_slot) {
            weights_[i] = new_gamma_p;
        } else {
            Float64 di = std::abs(d_direction[i]);
            Float64 candidate_w = (gamma_p * di) / pivot_d;
            weights_[i] = std::max(weights_[i], candidate_w);
        }
    }

    pivot_count_++;
    if (pivot_count_ >= reset_interval_) {
        reset_weights();
    }
}

} // namespace sovereign::lp
