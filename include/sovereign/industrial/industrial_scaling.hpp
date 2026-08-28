#ifndef SOVEREIGN_INDUSTRIAL_INDUSTRIAL_SCALING_HPP
#define SOVEREIGN_INDUSTRIAL_INDUSTRIAL_SCALING_HPP

#include "sovereign/lp/lp_model.hpp"
#include <vector>

namespace sovereign::industrial {

enum class ScalingPolicy {
    None,
    GeometricMean,
    RuizEquilibration
};

struct ScalingFactors {
    std::vector<Float64> row_scales;
    std::vector<Float64> col_scales;
};

class IndustrialScaling {
public:
    // Scale LP model in-place according to policy and return scaling vectors for postsolve unscaling
    static ScalingFactors scale_model(lp::LPModel& model, ScalingPolicy policy, std::size_t max_passes = 4);
    
    // Unscale primal solution vector x_scaled -> x_orig
    static std::vector<Float64> unscale_primal_solution(const std::vector<Float64>& x_scaled, const ScalingFactors& factors);
    
    // Unscale dual solution vector y_scaled -> y_orig
    static std::vector<Float64> unscale_dual_solution(const std::vector<Float64>& y_scaled, const ScalingFactors& factors);
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_INDUSTRIAL_SCALING_HPP
