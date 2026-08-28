#include "sovereign/presolve/postsolve_engine.hpp"
#include <cmath>
#include <algorithm>
#include <map>
#include <iostream>

namespace sovereign::presolve {

lp::SimplexResult PostsolveEngine::postsolve(
    const lp::LPModel& orig_model,
    const lp::LPModel& presolved_model,
    const lp::SimplexResult& presolved_result,
    const PresolveStack& stack
) const {
    lp::SimplexResult final_res = presolved_result;

    if (presolved_result.status != lp::SimplexStatus::Optimal) {
        return final_res;
    }

    Index n_orig = orig_model.num_variables();
    Index m_orig = orig_model.num_constraints();
    Index n_pres = presolved_model.num_variables();

    std::vector<Float64> x_orig(n_orig, 0.0);

    // 1. Map presolved variables back to original variables by name
    std::map<std::string, Index> orig_var_map;
    for (Index j = 0; j < n_orig; ++j) {
        orig_var_map[orig_model.get_variable(j).name] = j;
    }

    const auto& x_pres = presolved_result.primal_solution;
    for (Index j_pres = 0; j_pres < n_pres; ++j_pres) {
        const std::string& name = presolved_model.get_variable(j_pres).name;
        auto it = orig_var_map.find(name);
        if (it != orig_var_map.end()) {
            Index j_orig = it->second;
            Float64 val = (j_pres < static_cast<Index>(x_pres.size())) ? x_pres[j_pres] : 0.0;
            x_orig[j_orig] = val;
        }
    }

    // 2. Unwind PresolveStack in LIFO order (reverse of presolve order)
    const auto& reds = stack.reductions();
    for (auto it = reds.rbegin(); it != reds.rend(); ++it) {
        const auto& red = *it;

        switch (red.type) {
            case ReductionType::FixedVariable:
            case ReductionType::EmptyColumn: {
                if (red.col_index >= 0 && red.col_index < n_orig) {
                    x_orig[red.col_index] = red.fixed_value;
                }
                break;
            }
            case ReductionType::RowSingleton:
            case ReductionType::BoundTightening: {
                // Variable value is already in x_orig, bounds were tightened during presolve
                break;
            }
            case ReductionType::FreeColumnSingleton: {
                // x_j = (b_i - sum_{k!=j} a_ik x_k) / a_ij
                if (red.col_index >= 0 && red.col_index < n_orig) {
                    Float64 rhs = red.old_rhs;
                    Float64 a_ij = 0.0;
                    Float64 sum_other = 0.0;

                    for (std::size_t idx = 0; idx < red.col_indices.size(); ++idx) {
                        Index k = red.col_indices[idx];
                        Float64 coeff = red.coefficients[idx];
                        if (k == red.col_index) {
                            a_ij = coeff;
                        } else if (k >= 0 && k < n_orig) {
                            sum_other += coeff * x_orig[k];
                        }
                    }

                    if (std::abs(a_ij) > 1e-12) {
                        x_orig[red.col_index] = (rhs - sum_other) / a_ij;
                    }
                }
                break;
            }
            case ReductionType::EmptyRow:
            case ReductionType::RedundantRow: {
                break;
            }
        }
    }

    // 3. Compute original model primal residuals and feasibility
    Float64 max_primal_residual = 0.0;
    const auto& csc = orig_model.get_csc_matrix();

    std::vector<Float64> ax(m_orig, 0.0);
    for (Index j = 0; j < n_orig; ++j) {
        auto r_idx = csc.get_col_row_indices(j);
        auto vals = csc.get_col_values(j);
        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            ax[r] += vals[k] * x_orig[j];
        }
    }

    for (Index i = 0; i < m_orig; ++i) {
        const auto& c = orig_model.get_constraint(i);
        Float64 viol = 0.0;
        if (c.sense == lp::ConstraintSense::LessEqual) {
            if (ax[i] > c.rhs + tol_primal_) viol = ax[i] - c.rhs;
        } else if (c.sense == lp::ConstraintSense::GreaterEqual) {
            if (ax[i] < c.lhs - tol_primal_) viol = c.lhs - ax[i];
        } else if (c.sense == lp::ConstraintSense::Equal) {
            viol = std::abs(ax[i] - c.rhs);
        } else if (c.sense == lp::ConstraintSense::Range) {
            if (ax[i] > c.rhs + tol_primal_) viol = ax[i] - c.rhs;
            else if (ax[i] < c.lhs - tol_primal_) viol = c.lhs - ax[i];
        }

        if (viol > max_primal_residual) {
            max_primal_residual = viol;
        }
    }

    // Check bound feasibility
    for (Index j = 0; j < n_orig; ++j) {
        const auto& v = orig_model.get_variable(j);
        if (v.lower_bound > -INFINITY_VAL / 2.0) {
            if (x_orig[j] < v.lower_bound - tol_primal_) {
                Float64 viol = v.lower_bound - x_orig[j];
                if (viol > max_primal_residual) max_primal_residual = viol;
            }
        }
        if (v.upper_bound < INFINITY_VAL / 2.0) {
            if (x_orig[j] > v.upper_bound + tol_primal_) {
                Float64 viol = x_orig[j] - v.upper_bound;
                if (viol > max_primal_residual) max_primal_residual = viol;
            }
        }
    }

    // Calculate exact original objective value
    Float64 orig_obj = orig_model.objective_offset();
    for (Index j = 0; j < n_orig; ++j) {
        orig_obj += orig_model.get_variable(j).obj_coeff * x_orig[j];
    }

    final_res.primal_solution = std::move(x_orig);
    final_res.objective_value = orig_obj;
    final_res.max_primal_residual = max_primal_residual;
    final_res.is_primal_feasible = (max_primal_residual <= tol_primal_);

    return final_res;
}

} // namespace sovereign::presolve
