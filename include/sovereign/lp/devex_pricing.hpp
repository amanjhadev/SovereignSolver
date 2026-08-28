#ifndef SOVEREIGN_LP_DEVEX_PRICING_HPP
#define SOVEREIGN_LP_DEVEX_PRICING_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include <span>
#include <vector>

namespace sovereign::lp {

class DevexPricing {
private:
    Index m_{0};
    std::vector<Float64> weights_; // Size m: Devex weights for basic rows
    Int64 pivot_count_{0};
    Int64 reset_interval_{500};

public:
    DevexPricing() = default;
    explicit DevexPricing(Index num_rows);

    void reset_weights();
    Index select_leaving_variable(const SimplexState& state, Float64 tol_primal, Float64& max_weighted_viol) const;
    void update_weights(Index leaving_slot, std::span<const Float64> d_direction);
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_DEVEX_PRICING_HPP
