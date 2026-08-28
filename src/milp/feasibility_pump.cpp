#include "sovereign/milp/feasibility_pump.hpp"
#include "sovereign/milp/branching.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include <cmath>
#include <random>
#include <algorithm>

namespace sovereign::milp {

FeasibilityPumpHeuristic::FeasibilityPumpHeuristic(Int32 max_iterations, Float64 tol_integrality)
    : max_iterations_(max_iterations), tol_integrality_(tol_integrality) {}

HeuristicResult FeasibilityPumpHeuristic::run(
    const MILPModel& model,
    std::span<const Float64> root_lp_sol
) const {
    HeuristicResult res;
    res.found_solution = false;

    Index n_orig = model.num_variables();
    if (n_orig == 0 || root_lp_sol.empty()) return res;

    std::vector<Index> integer_var_indices = model.integer_variable_indices();
    if (integer_var_indices.empty()) return res;

    std::vector<Float64> current_x(root_lp_sol.begin(), root_lp_sol.end());
    std::vector<Float64> target(n_orig);

    std::mt19937 rng(42);
    std::uniform_real_distribution<Float64> dist(0.0, 1.0);

    for (Int32 iter = 0; iter < max_iterations_; ++iter) {
        // Round current solution to nearest integer for integer variables
        bool all_integral = true;
        for (Index j : integer_var_indices) {
            Float64 val = current_x[j];
            Float64 rounded = std::floor(val + 0.5);
            if (std::abs(val - rounded) > tol_integrality_) {
                all_integral = false;
            }
            target[j] = rounded;
        }

        for (Index j = 0; j < n_orig; ++j) {
            if (!model.is_integer(j)) {
                target[j] = current_x[j];
            }
        }

        // Check if current_x is ALREADY integer feasible in original MILP
        if (all_integral && check_solution_integrality(current_x, integer_var_indices, tol_integrality_)) {
            // Verify feasibility against constraints
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

                Float64 obj = model.objective_offset();
                for (Index j = 0; j < n_orig; ++j) {
                    obj += model.lp_model().get_variable(j).obj_coeff * current_x[j];
                }
                res.objective_value = obj;
                return res;
            }
        }

        // Perturb target if cycling is detected
        if (iter > 0 && iter % 5 == 0) {
            for (Index j : integer_var_indices) {
                if (dist(rng) < 0.3) {
                    target[j] = (target[j] >= current_x[j]) ? std::floor(current_x[j]) : std::ceil(current_x[j]);
                }
            }
        }

        // Construct L1 distance minimization LP relaxation: min \sum_{j in Int} d_j
        lp::LPModel fp_lp = model.lp_model();
        // Clear original objective
        for (Index j = 0; j < fp_lp.num_variables(); ++j) {
            fp_lp.get_variable(j).obj_coeff = 0.0;
        }
        fp_lp.set_objective_offset(0.0);

        for (Index j : integer_var_indices) {
            Index d_j = fp_lp.add_variable("d_" + std::to_string(j), 0.0, INFINITY_VAL, 1.0, VariableType::Continuous);
            Float64 t_j = target[j];

            std::vector<Index> pos_cols = {d_j, j};
            std::vector<Float64> pos_vals = {1.0, -1.0};
            fp_lp.add_constraint(
                "fp_l1_pos_" + std::to_string(j),
                lp::ConstraintSense::GreaterEqual,
                -t_j,
                pos_cols,
                pos_vals
            );

            std::vector<Index> neg_cols = {d_j, j};
            std::vector<Float64> neg_vals = {1.0, 1.0};
            fp_lp.add_constraint(
                "fp_l1_neg_" + std::to_string(j),
                lp::ConstraintSense::GreaterEqual,
                t_j,
                neg_cols,
                neg_vals
            );
        }

        lp::DualSimplexSolver fp_solver;
        auto fp_res = fp_solver.solve(fp_lp);
        if (fp_res.status != lp::SimplexStatus::Optimal) {
            break; // Failed to solve distance LP
        }

        current_x = fp_res.primal_solution;
        if (current_x.size() > static_cast<std::size_t>(n_orig)) {
            current_x.resize(n_orig);
        }
    }

    return res;
}

} // namespace sovereign::milp
