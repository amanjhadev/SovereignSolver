#ifndef SOVEREIGN_LP_RATIO_TEST_HPP
#define SOVEREIGN_LP_RATIO_TEST_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include <span>
#include <vector>

namespace sovereign::lp {

struct RatioTestResult {
    Index entering_col{-1};
    Float64 step_length{0.0};
    bool is_bound_flip{false};
    bool valid{false};
};

class DualRatioTest {
private:
    Float64 tol_primal_{1e-6};
    Float64 tol_dual_{1e-7};
    Float64 tol_pivot_{1e-9};
    Float64 tol_harris_{1e-6};

public:
    DualRatioTest() = default;
    DualRatioTest(Float64 tol_primal, Float64 tol_dual, Float64 tol_pivot = 1e-9, Float64 tol_harris = 1e-6);

    // Baseline Dual Ratio Test
    RatioTestResult select_entering_variable(
        const SimplexState& state,
        Index leaving_slot,
        std::span<const Float64> row_alphas
    ) const;

    // Harris Two-Pass Dual Ratio Test
    RatioTestResult select_entering_variable_harris(
        const SimplexState& state,
        Index leaving_slot,
        std::span<const Float64> row_alphas
    ) const;
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_RATIO_TEST_HPP
