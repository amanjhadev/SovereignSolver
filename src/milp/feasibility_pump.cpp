#include "sovereign/milp/feasibility_pump.hpp"
#include "sovereign/milp/branching.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <set>
#include <chrono>

namespace sovereign::milp {

FeasibilityPumpHeuristic::FeasibilityPumpHeuristic(Int32 max_iterations, Float64 tol_integrality)
    : max_iterations_(max_iterations), tol_integrality_(tol_integrality) {}

HeuristicResult FeasibilityPumpHeuristic::run(
    const MILPModel& model,
    std::span<const Float64> root_lp_sol
) const {
    FeasibilityPumpTelemetry telemetry;
    return run_with_telemetry(model, root_lp_sol, telemetry);
}

HeuristicResult FeasibilityPumpHeuristic::run_with_telemetry(
    const MILPModel& model,
    std::span<const Float64> root_lp_sol,
    FeasibilityPumpTelemetry& telemetry
) const {
    HeuristicResult res;
    res.found_solution = false;

    Index n_orig = model.num_variables();
    if (n_orig == 0 || root_lp_sol.empty()) return res;

    std::vector<Index> integer_var_indices = model.integer_variable_indices();
    if (integer_var_indices.empty()) return res;

    // Separate Binary variables vs General Integer variables
    std::vector<Index> binary_indices;
    std::vector<Index> general_int_indices;

    for (Index j : integer_var_indices) {
        Float64 lb = model.lp_model().get_variable(j).lower_bound;
        Float64 ub = model.lp_model().get_variable(j).upper_bound;
        if (std::abs(lb - 0.0) <= tol_integrality_ && std::abs(ub - 1.0) <= tol_integrality_) {
            binary_indices.push_back(j);
        } else {
            general_int_indices.push_back(j);
        }
    }

    // Save Original Objective Lifecycle
    lp::LPModel fp_lp = model.lp_model();
    std::vector<Float64> orig_obj_coeffs(n_orig);
    for (Index j = 0; j < n_orig; ++j) {
        orig_obj_coeffs[j] = fp_lp.get_variable(j).obj_coeff;
    }
    Float64 orig_obj_offset = fp_lp.objective_offset();

    // Add distance variables ONLY for General Integer variables
    std::vector<Index> d_var_indices;
    for (Index k : general_int_indices) {
        Index d_k = fp_lp.add_variable("d_gen_" + std::to_string(k), 0.0, INFINITY_VAL, 1.0, VariableType::Continuous);
        d_var_indices.push_back(d_k);

        std::vector<Index> pos_cols = {d_k, k};
        std::vector<Float64> pos_vals = {1.0, -1.0};
        fp_lp.add_constraint("fp_pos_" + std::to_string(k), lp::ConstraintSense::GreaterEqual, 0.0, pos_cols, pos_vals);

        std::vector<Index> neg_cols = {d_k, k};
        std::vector<Float64> neg_vals = {1.0, 1.0};
        fp_lp.add_constraint("fp_neg_" + std::to_string(k), lp::ConstraintSense::GreaterEqual, 0.0, neg_cols, neg_vals);
    }

    // Record Telemetry Model Scale
    telemetry.auxiliary_vars = fp_lp.num_variables() - n_orig;
    telemetry.auxiliary_constraints = fp_lp.num_constraints() - model.num_constraints();
    const auto& csc_mat = fp_lp.get_csc_matrix();
    Int64 nnz_cnt = 0;
    for (Index j = 0; j < fp_lp.num_variables(); ++j) {
        nnz_cnt += csc_mat.get_col_values(j).size();
    }
    telemetry.auxiliary_nnz = nnz_cnt;

    std::vector<Float64> current_x(root_lp_sol.begin(), root_lp_sol.end());
    std::vector<Float64> target(n_orig);

    std::mt19937 rng(42);
    std::uniform_real_distribution<Float64> dist(0.0, 1.0);

    std::set<std::vector<Int32>> seen_targets;
    std::optional<lp::Basis> warm_start_basis;
    Float64 prev_l1_dist = INFINITY_VAL;
    Int32 no_progress_cnt = 0;

    for (Int32 iter = 0; iter < max_iterations_; ++iter) {
        telemetry.iterations_run++;

        // 1. Round current solution for integer variables
        bool all_integral = true;
        Float64 current_l1_dist = 0.0;
        std::vector<Int32> target_discrete(integer_var_indices.size());

        for (std::size_t idx = 0; idx < integer_var_indices.size(); ++idx) {
            Index j = integer_var_indices[idx];
            Float64 val = current_x[j];
            Float64 rounded = std::floor(val + 0.5);
            Float64 dev = std::abs(val - rounded);
            if (dev > tol_integrality_) {
                all_integral = false;
            }
            current_l1_dist += dev;
            target[j] = rounded;
            target_discrete[idx] = static_cast<Int32>(rounded);
        }

        for (Index j = 0; j < n_orig; ++j) {
            if (!model.is_integer(j)) {
                target[j] = current_x[j];
            }
        }

        telemetry.best_l1_distance = std::min(telemetry.best_l1_distance, current_l1_dist);

        // 2. Check if current_x is ALREADY integer feasible in original MILP
        if (all_integral && check_solution_integrality(current_x, integer_var_indices, tol_integrality_)) {
            // Verify feasibility against original constraints
            lp::DualSimplexSolver verifier;
            auto val_lp = model.lp_model();
            for (Index j = 0; j < n_orig; ++j) {
                val_lp.get_variable(j).lower_bound = current_x[j];
                val_lp.get_variable(j).upper_bound = current_x[j];
            }
            auto ver_res = verifier.solve(val_lp);
            if (ver_res.status == lp::SimplexStatus::Optimal) {
                res.found_solution = true;
                res.solution = current_x;
                res.solution.resize(n_orig);

                Float64 true_obj = orig_obj_offset;
                for (Index j = 0; j < n_orig; ++j) {
                    true_obj += orig_obj_coeffs[j] * current_x[j];
                }
                res.objective_value = true_obj;

                telemetry.found_incumbent = true;
                telemetry.incumbent_objective = true_obj;

                // Restore original objective before returning
                for (Index j = 0; j < n_orig; ++j) {
                    fp_lp.get_variable(j).obj_coeff = orig_obj_coeffs[j];
                }
                fp_lp.set_objective_offset(orig_obj_offset);
                return res;
            }
        }

        // 3. Cycle & No-Progress Detection
        if (seen_targets.find(target_discrete) != seen_targets.end()) {
            telemetry.cycle_detections++;
            // Perturb target vector
            for (Index j : binary_indices) {
                if (dist(rng) < 0.3) {
                    target[j] = (target[j] >= current_x[j]) ? std::floor(current_x[j]) : std::ceil(current_x[j]);
                }
            }
        } else {
            seen_targets.insert(target_discrete);
        }

        if (current_l1_dist >= prev_l1_dist - 1e-6) {
            no_progress_cnt++;
        } else {
            no_progress_cnt = 0;
            prev_l1_dist = current_l1_dist;
        }

        if (no_progress_cnt >= 5) {
            break; // Stop pump if no integrality improvement for 5 consecutive iterations
        }

        // 4. Update Objective Costs In-Place
        auto t_u1 = std::chrono::high_resolution_clock::now();

        // Clear all objective coefficients
        for (Index j = 0; j < fp_lp.num_variables(); ++j) {
            fp_lp.get_variable(j).obj_coeff = 0.0;
        }
        Float64 offset_shift = 0.0;

        // Binary variables: Exact in-place objective assignment (c'_j = +1 if t_j=0, c'_j = -1 if t_j=1)
        for (Index j : binary_indices) {
            Float64 t_j = target[j];
            if (std::abs(t_j - 0.0) <= tol_integrality_) {
                fp_lp.get_variable(j).obj_coeff = 1.0;
            } else {
                fp_lp.get_variable(j).obj_coeff = -1.0;
                offset_shift += 1.0;
            }
        }
        fp_lp.set_objective_offset(offset_shift);

        // General integer variables: Objective coefficient 1.0 on auxiliary d_k
        for (Index d_k : d_var_indices) {
            fp_lp.get_variable(d_k).obj_coeff = 1.0;
        }
        // Update general integer constraint RHS values for d_k >= +- (x_k - t_k)
        for (std::size_t idx = 0; idx < general_int_indices.size(); ++idx) {
            Index k = general_int_indices[idx];
            Float64 t_k = target[k];
            Index pos_row = model.num_constraints() + 2 * idx;
            Index neg_row = model.num_constraints() + 2 * idx + 1;
            fp_lp.get_constraint(pos_row).lhs = -t_k;
            fp_lp.get_constraint(neg_row).lhs = t_k;
        }

        auto t_u2 = std::chrono::high_resolution_clock::now();
        telemetry.total_obj_update_time_sec += std::chrono::duration<double>(t_u2 - t_u1).count();

        // 5. Solve Projection LP with Warm-Start & Fallback
        auto t_s1 = std::chrono::high_resolution_clock::now();
        lp::DualSimplexSolver fp_solver;
        lp::SimplexResult fp_res;

        if (warm_start_basis.has_value()) {
            telemetry.warm_start_attempts++;
            fp_res = fp_solver.solve(fp_lp, warm_start_basis.value());
            if (fp_res.status == lp::SimplexStatus::Optimal) {
                telemetry.warm_start_accepted++;
            } else {
                telemetry.warm_start_rejected++;
                fp_res = fp_solver.solve(fp_lp); // Fallback to clean solve
            }
        } else {
            fp_res = fp_solver.solve(fp_lp); // Clean initial solve
        }

        auto t_s2 = std::chrono::high_resolution_clock::now();
        telemetry.total_lp_solve_time_sec += std::chrono::duration<double>(t_s2 - t_s1).count();
        telemetry.total_lp_iterations += fp_res.iterations;

        if (fp_res.status != lp::SimplexStatus::Optimal) {
            break; // Distance LP failed to solve
        }

        warm_start_basis = fp_res.basis;
        current_x = fp_res.primal_solution;
        if (current_x.size() > static_cast<std::size_t>(n_orig)) {
            current_x.resize(n_orig);
        }
    }

    // Restore Original Objective Lifecycle before returning
    for (Index j = 0; j < n_orig; ++j) {
        fp_lp.get_variable(j).obj_coeff = orig_obj_coeffs[j];
    }
    fp_lp.set_objective_offset(orig_obj_offset);

    return res;
}

} // namespace sovereign::milp
