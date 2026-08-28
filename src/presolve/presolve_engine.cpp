#include "sovereign/presolve/presolve_engine.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>

namespace sovereign::presolve {

PresolveResult PresolveEngine::presolve(const lp::LPModel& orig_model) const {
    PresolveResult result;
    result.status = PresolveStatus::Clean;

    Index m = orig_model.num_constraints();
    Index n = orig_model.num_variables();

    std::vector<bool> deleted_rows(m, false);
    std::vector<bool> deleted_cols(n, false);

    std::vector<Float64> var_lb(n);
    std::vector<Float64> var_ub(n);
    std::vector<Float64> obj_coeff(n);
    for (Index j = 0; j < n; ++j) {
        const auto& v = orig_model.get_variable(j);
        var_lb[j] = v.lower_bound;
        var_ub[j] = v.upper_bound;
        obj_coeff[j] = v.obj_coeff;
    }

    Float64 obj_offset = orig_model.objective_offset();

    std::vector<Float64> row_lhs(m);
    std::vector<Float64> row_rhs(m);
    std::vector<lp::ConstraintSense> row_sense(m);
    for (Index i = 0; i < m; ++i) {
        const auto& c = orig_model.get_constraint(i);
        row_lhs[i] = c.lhs;
        row_rhs[i] = c.rhs;
        row_sense[i] = c.sense;
    }

    // Build internal sparse representation: row_adj[i] and col_adj[j]
    std::vector<std::vector<std::pair<Index, Float64>>> row_adj(m);
    std::vector<std::vector<std::pair<Index, Float64>>> col_adj(n);

    const auto& csc = orig_model.get_csc_matrix();
    for (Index j = 0; j < n; ++j) {
        auto r_idx = csc.get_col_row_indices(j);
        auto vals = csc.get_col_values(j);
        for (std::size_t k = 0; k < r_idx.size(); ++k) {
            Index r = r_idx[k];
            Float64 val = vals[k];
            row_adj[r].push_back({j, val});
            col_adj[j].push_back({r, val});
        }
    }

    Float64 tol = config_.tolerance;
    Float64 piv_tol = config_.pivot_tolerance;
    bool is_min = (orig_model.objective_sense() == ObjectiveSense::Minimize);

    Int32 passes = 0;
    Int64 total_reductions = 0;

    for (passes = 0; passes < config_.max_passes; ++passes) {
        Int64 reductions_in_pass = 0;

        // 1. Fixed Variable Elimination
        for (Index j = 0; j < n; ++j) {
            if (deleted_cols[j]) continue;
            if (std::abs(var_ub[j] - var_lb[j]) <= tol) {
                Float64 fix_val = var_lb[j];

                PresolveReduction red;
                red.type = ReductionType::FixedVariable;
                red.col_index = j;
                red.var_name = orig_model.get_variable(j).name;
                red.fixed_value = fix_val;
                red.old_lower_bound = var_lb[j];
                red.old_upper_bound = var_ub[j];
                red.objective_coeff = obj_coeff[j];

                obj_offset += obj_coeff[j] * fix_val;

                for (const auto& edge : col_adj[j]) {
                    Index r = edge.first;
                    Float64 val = edge.second;
                    red.col_indices.push_back(r);
                    red.coefficients.push_back(val);

                    if (!deleted_rows[r]) {
                        if (row_lhs[r] > -INFINITY_VAL / 2.0) row_lhs[r] -= val * fix_val;
                        if (row_rhs[r] < INFINITY_VAL / 2.0) row_rhs[r] -= val * fix_val;
                    }
                }

                deleted_cols[j] = true;
                result.stack.push_reduction(std::move(red));
                reductions_in_pass++;
            }
        }

        // 2. Redundant Row / Empty Row Elimination
        for (Index i = 0; i < m; ++i) {
            if (deleted_rows[i]) continue;
            Index active_count = 0;
            for (const auto& edge : row_adj[i]) {
                if (!deleted_cols[edge.first]) active_count++;
            }

            if (active_count == 0) {
                if (row_lhs[i] > tol || row_rhs[i] < -tol) {
                    result.status = PresolveStatus::Infeasible;
                    result.message = "Empty row " + orig_model.get_constraint(i).name + " is infeasible";
                    return result;
                }

                PresolveReduction red;
                red.type = ReductionType::EmptyRow;
                red.row_index = i;
                red.constraint_name = orig_model.get_constraint(i).name;
                red.old_lhs = row_lhs[i];
                red.old_rhs = row_rhs[i];
                red.old_sense = row_sense[i];

                deleted_rows[i] = true;
                result.stack.push_reduction(std::move(red));
                reductions_in_pass++;
            }
        }

        // 3. Empty Column Elimination (Unconstrained Variable Fix)
        if (config_.enable_empty_cols) {
            for (Index j = 0; j < n; ++j) {
                if (deleted_cols[j]) continue;
                Index active_count = 0;
                for (const auto& edge : col_adj[j]) {
                    if (!deleted_rows[edge.first]) active_count++;
                }

                if (active_count == 0) {
                    Float64 c = obj_coeff[j];
                    Float64 fix_val = 0.0;
                    bool unbounded = false;

                    if (is_min) {
                        if (c > tol) {
                            if (var_lb[j] <= -INFINITY_VAL / 2.0) unbounded = true;
                            else fix_val = var_lb[j];
                        } else if (c < -tol) {
                            if (var_ub[j] >= INFINITY_VAL / 2.0) unbounded = true;
                            else fix_val = var_ub[j];
                        } else {
                            fix_val = (var_lb[j] > -INFINITY_VAL / 2.0) ? var_lb[j] : ((var_ub[j] < INFINITY_VAL / 2.0) ? var_ub[j] : 0.0);
                        }
                    } else { // Maximize
                        if (c > tol) {
                            if (var_ub[j] >= INFINITY_VAL / 2.0) unbounded = true;
                            else fix_val = var_ub[j];
                        } else if (c < -tol) {
                            if (var_lb[j] <= -INFINITY_VAL / 2.0) unbounded = true;
                            else fix_val = var_lb[j];
                        } else {
                            fix_val = (var_lb[j] > -INFINITY_VAL / 2.0) ? var_lb[j] : ((var_ub[j] < INFINITY_VAL / 2.0) ? var_ub[j] : 0.0);
                        }
                    }

                    if (unbounded) {
                        result.status = PresolveStatus::Unbounded;
                        result.message = "Empty column " + orig_model.get_variable(j).name + " is unbounded";
                        return result;
                    }

                    PresolveReduction red;
                    red.type = ReductionType::EmptyColumn;
                    red.col_index = j;
                    red.var_name = orig_model.get_variable(j).name;
                    red.fixed_value = fix_val;
                    red.old_lower_bound = var_lb[j];
                    red.old_upper_bound = var_ub[j];
                    red.objective_coeff = c;

                    obj_offset += c * fix_val;
                    deleted_cols[j] = true;
                    result.stack.push_reduction(std::move(red));
                    reductions_in_pass++;
                }
            }
        }

        // 4. Row Singleton Bound Tightening
        if (config_.enable_row_singleton) {
            for (Index i = 0; i < m; ++i) {
            if (deleted_rows[i]) continue;
            Index active_col = -1;
            Float64 coeff = 0.0;
            Index active_count = 0;

            for (const auto& edge : row_adj[i]) {
                if (!deleted_cols[edge.first]) {
                    active_count++;
                    active_col = edge.first;
                    coeff = edge.second;
                }
            }

            if (active_count == 1 && std::abs(coeff) >= piv_tol) {
                Index j = active_col;
                Float64 L_j = -INFINITY_VAL;
                Float64 U_j = INFINITY_VAL;

                if (coeff > 0) {
                    if (row_lhs[i] > -INFINITY_VAL / 2.0) L_j = row_lhs[i] / coeff;
                    if (row_rhs[i] < INFINITY_VAL / 2.0) U_j = row_rhs[i] / coeff;
                } else {
                    if (row_rhs[i] < INFINITY_VAL / 2.0) L_j = row_rhs[i] / coeff;
                    if (row_lhs[i] > -INFINITY_VAL / 2.0) U_j = row_lhs[i] / coeff;
                }

                Float64 new_lb = std::max(var_lb[j], L_j);
                Float64 new_ub = std::min(var_ub[j], U_j);

                if (new_lb > new_ub + tol) {
                    result.status = PresolveStatus::Infeasible;
                    result.message = "Row singleton " + orig_model.get_constraint(i).name + " created infeasible variable bounds";
                    return result;
                }

                PresolveReduction red;
                red.type = ReductionType::RowSingleton;
                red.row_index = i;
                red.col_index = j;
                red.constraint_name = orig_model.get_constraint(i).name;
                red.var_name = orig_model.get_variable(j).name;
                red.old_lower_bound = var_lb[j];
                red.old_upper_bound = var_ub[j];
                red.old_lhs = row_lhs[i];
                red.old_rhs = row_rhs[i];
                red.old_sense = row_sense[i];

                var_lb[j] = new_lb;
                var_ub[j] = new_ub;
                deleted_rows[i] = true;
                result.stack.push_reduction(std::move(red));
                reductions_in_pass++;
            }
        }
        }

        // 5. Free Column Singleton Substitution
        if (config_.enable_free_col_singleton) {
            for (Index j = 0; j < n; ++j) {
                if (deleted_cols[j]) continue;
                if (var_lb[j] > -INFINITY_VAL / 2.0 || var_ub[j] < INFINITY_VAL / 2.0) continue; // Must be unconstrained free variable

                Index singleton_row = -1;
                Float64 coeff = 0.0;
                Index active_count = 0;

                for (const auto& edge : col_adj[j]) {
                    Index r = edge.first;
                    if (!deleted_rows[r]) {
                        active_count++;
                        singleton_row = r;
                        coeff = edge.second;
                    }
                }

                if (active_count == 1 && std::abs(coeff) >= piv_tol) {
                    Index i = singleton_row;

                    // Must be equality constraint
                    if (row_sense[i] != lp::ConstraintSense::Equal) continue;

                    Float64 b_i = row_rhs[i];

                    PresolveReduction red;
                    red.type = ReductionType::FreeColumnSingleton;
                    red.col_index = j;
                    red.row_index = i;
                    red.var_name = orig_model.get_variable(j).name;
                    red.constraint_name = orig_model.get_constraint(i).name;
                    red.objective_coeff = obj_coeff[j];
                    red.old_rhs = b_i;

                    // Substitute x_j = (b_i - sum_{k!=j} a_ik x_k) / coeff into objective
                    Float64 c_j = obj_coeff[j];
                    obj_offset += c_j * (b_i / coeff);

                    for (const auto& edge : row_adj[i]) {
                        Index k = edge.first;
                        Float64 a_ik = edge.second;
                        red.col_indices.push_back(k);
                        red.coefficients.push_back(a_ik);

                        if (k != j && !deleted_cols[k]) {
                            obj_coeff[k] -= c_j * (a_ik / coeff);
                        }
                    }

                    deleted_cols[j] = true;
                    deleted_rows[i] = true;
                    result.stack.push_reduction(std::move(red));
                    reductions_in_pass++;
                }
            }
        }

        // 6. Safe Implied Bound Tightening & Redundant Row Elimination
        for (Index i = 0; i < m; ++i) {
            if (deleted_rows[i]) continue;

            Float64 row_L = 0.0;
            Float64 row_U = 0.0;
            bool L_inf = false;
            bool U_inf = false;

            for (const auto& edge : row_adj[i]) {
                Index j = edge.first;
                if (deleted_cols[j]) continue;
                Float64 a_ij = edge.second;

                if (a_ij > 0) {
                    if (var_lb[j] <= -INFINITY_VAL / 2.0) L_inf = true;
                    else row_L += a_ij * var_lb[j];

                    if (var_ub[j] >= INFINITY_VAL / 2.0) U_inf = true;
                    else row_U += a_ij * var_ub[j];
                } else {
                    if (var_ub[j] >= INFINITY_VAL / 2.0) L_inf = true;
                    else row_L += a_ij * var_ub[j];

                    if (var_lb[j] <= -INFINITY_VAL / 2.0) U_inf = true;
                    else row_U += a_ij * var_lb[j];
                }
            }

            // Check constraint feasibility
            if (!U_inf && row_lhs[i] > -INFINITY_VAL / 2.0 && row_U < row_lhs[i] - tol) {
                result.status = PresolveStatus::Infeasible;
                result.message = "Constraint " + orig_model.get_constraint(i).name + " upper implied bound below LHS";
                return result;
            }
            if (!L_inf && row_rhs[i] < INFINITY_VAL / 2.0 && row_L > row_rhs[i] + tol) {
                result.status = PresolveStatus::Infeasible;
                result.message = "Constraint " + orig_model.get_constraint(i).name + " lower implied bound above RHS";
                return result;
            }

            // Redundant Row Elimination: if implied activity range is strictly within bounds
            bool redundant_lhs = (row_lhs[i] <= -INFINITY_VAL / 2.0) || (!L_inf && row_L >= row_lhs[i] - tol);
            bool redundant_rhs = (row_rhs[i] >= INFINITY_VAL / 2.0) || (!U_inf && row_U <= row_rhs[i] + tol);

            if (redundant_lhs && redundant_rhs) {
                PresolveReduction red;
                red.type = ReductionType::RedundantRow;
                red.row_index = i;
                red.constraint_name = orig_model.get_constraint(i).name;
                red.old_lhs = row_lhs[i];
                red.old_rhs = row_rhs[i];
                red.old_sense = row_sense[i];

                deleted_rows[i] = true;
                result.stack.push_reduction(std::move(red));
                reductions_in_pass++;
            }
        }

        total_reductions += reductions_in_pass;
        if (reductions_in_pass == 0) break; // Reached presolve fixed point
    }

    // Construct reduced presolved_model
    lp::LPModel presolved(orig_model.name() + "_presolved", orig_model.objective_sense());
    presolved.set_objective_offset(obj_offset);

    std::vector<Index> orig_to_presolved_col(n, -1);
    for (Index j = 0; j < n; ++j) {
        if (!deleted_cols[j]) {
            const auto& orig_v = orig_model.get_variable(j);
            Index new_j = presolved.add_variable(orig_v.name, var_lb[j], var_ub[j], obj_coeff[j], orig_v.type);
            presolved.get_variable(new_j).original_var_index = j;
            orig_to_presolved_col[j] = new_j;
        }
    }

    for (Index i = 0; i < m; ++i) {
        if (!deleted_rows[i]) {
            std::vector<Index> r_cols;
            std::vector<Float64> r_vals;
            for (const auto& edge : row_adj[i]) {
                Index j = edge.first;
                if (!deleted_cols[j]) {
                    r_cols.push_back(orig_to_presolved_col[j]);
                    r_vals.push_back(edge.second);
                }
            }

            const auto& orig_c = orig_model.get_constraint(i);
            if (row_sense[i] == lp::ConstraintSense::Range) {
                presolved.add_constraint_range(orig_c.name, row_lhs[i], row_rhs[i], r_cols, r_vals);
            } else {
                Float64 bound_val = (row_sense[i] == lp::ConstraintSense::GreaterEqual) ? row_lhs[i] : row_rhs[i];
                presolved.add_constraint(orig_c.name, row_sense[i], bound_val, r_cols, r_vals);
            }
        }
    }

    result.presolved_model = std::move(presolved);
    result.passes_executed = passes + 1;
    result.total_reductions = total_reductions;
    result.message = "Presolve completed successfully";

    return result;
}

} // namespace sovereign::presolve
