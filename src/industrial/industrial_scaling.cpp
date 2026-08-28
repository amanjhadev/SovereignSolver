#include "sovereign/industrial/industrial_scaling.hpp"
#include <cmath>
#include <algorithm>

namespace sovereign::industrial {

ScalingFactors IndustrialScaling::scale_model(lp::LPModel& model, ScalingPolicy policy, std::size_t max_passes) {
    ScalingFactors sf;
    std::size_t m = model.num_constraints();
    std::size_t n = model.num_variables();
    sf.row_scales.assign(m, 1.0);
    sf.col_scales.assign(n, 1.0);

    if (policy == ScalingPolicy::None || m == 0 || n == 0) {
        return sf;
    }

    const auto& matrix = model.get_csc_matrix();

    for (std::size_t pass = 0; pass < max_passes; ++pass) {
        // Row scaling pass
        std::vector<Float64> row_max(m, 0.0);
        std::vector<Float64> row_min(m, 1e20);

        for (Index j = 0; j < static_cast<Index>(n); ++j) {
            auto rows = matrix.get_col_row_indices(j);
            auto vals = matrix.get_col_values(j);
            for (std::size_t p = 0; p < rows.size(); ++p) {
                Index r = rows[p];
                Float64 val = std::abs(vals[p]);
                if (val > 0.0) {
                    row_max[r] = std::max(row_max[r], val);
                    row_min[r] = std::min(row_min[r], val);
                }
            }
        }

        for (Index i = 0; i < static_cast<Index>(m); ++i) {
            Float64 r_factor = 1.0;
            if (policy == ScalingPolicy::RuizEquilibration) {
                if (row_max[i] > 0.0) r_factor = 1.0 / std::sqrt(row_max[i]);
            } else if (policy == ScalingPolicy::GeometricMean) {
                if (row_max[i] > 0.0 && row_min[i] < 1e19) r_factor = 1.0 / std::sqrt(row_max[i] * row_min[i]);
            }

            if (r_factor != 1.0) {
                sf.row_scales[i] *= r_factor;
                model.get_constraint(i).rhs *= r_factor;
            }
        }

        // Col scaling pass
        for (Index j = 0; j < static_cast<Index>(n); ++j) {
            auto vals = matrix.get_col_values(j);
            Float64 col_max = 0.0;
            Float64 col_min = 1e20;
            for (Float64 val_raw : vals) {
                Float64 val = std::abs(val_raw);
                if (val > 0.0) {
                    col_max = std::max(col_max, val);
                    col_min = std::min(col_min, val);
                }
            }

            Float64 c_factor = 1.0;
            if (policy == ScalingPolicy::RuizEquilibration) {
                if (col_max > 0.0) c_factor = 1.0 / std::sqrt(col_max);
            } else if (policy == ScalingPolicy::GeometricMean) {
                if (col_max > 0.0 && col_min < 1e19) c_factor = 1.0 / std::sqrt(col_max * col_min);
            }

            if (c_factor != 1.0) {
                sf.col_scales[j] *= c_factor;
                auto& var = model.get_variable(j);
                var.obj_coeff *= c_factor;
                if (var.lower_bound > -1e19) var.lower_bound /= c_factor;
                if (var.upper_bound < 1e19) var.upper_bound /= c_factor;
            }
        }
    }

    return sf;
}

std::vector<Float64> IndustrialScaling::unscale_primal_solution(const std::vector<Float64>& x_scaled, const ScalingFactors& sf) {
    std::vector<Float64> x_orig(x_scaled.size());
    for (std::size_t j = 0; j < x_scaled.size(); ++j) {
        Float64 c_scale = (j < sf.col_scales.size() && sf.col_scales[j] != 0.0) ? sf.col_scales[j] : 1.0;
        x_orig[j] = x_scaled[j] / c_scale;
    }
    return x_orig;
}

std::vector<Float64> IndustrialScaling::unscale_dual_solution(const std::vector<Float64>& y_scaled, const ScalingFactors& sf) {
    std::vector<Float64> y_orig(y_scaled.size());
    for (std::size_t i = 0; i < y_scaled.size(); ++i) {
        Float64 r_scale = (i < sf.row_scales.size() && sf.row_scales[i] != 0.0) ? sf.row_scales[i] : 1.0;
        y_orig[i] = y_scaled[i] / r_scale;
    }
    return y_orig;
}

} // namespace sovereign::industrial
