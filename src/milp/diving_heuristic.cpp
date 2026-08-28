#include "sovereign/milp/diving_heuristic.hpp"
#include "sovereign/milp/branching.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include <cmath>
#include <algorithm>

namespace sovereign::milp {

DivingHeuristic::DivingHeuristic(Int32 max_depth, Float64 tol_integrality)
    : max_depth_(max_depth), tol_integrality_(tol_integrality) {}

HeuristicResult DivingHeuristic::run(
    const MILPModel& model,
    const lp::LPModel& current_node_lp
) const {
    HeuristicResult res;
    res.found_solution = false;

    Index n_orig = model.num_variables();
    if (n_orig == 0) return res;

    std::vector<Index> integer_var_indices = model.integer_variable_indices();
    if (integer_var_indices.empty()) return res;

    lp::LPModel dive_lp = current_node_lp;
    lp::DualSimplexSolver solver;

    for (Int32 depth = 0; depth < max_depth_; ++depth) {
        auto dive_res = solver.solve(dive_lp);
        if (dive_res.status != lp::SimplexStatus::Optimal) {
            break; // Dive path hit infeasibility
        }

        const auto& sol = dive_res.primal_solution;

        // Check if current dive solution is fully integer feasible
        bool is_int = check_solution_integrality(sol, integer_var_indices, tol_integrality_);
        if (is_int) {
            res.found_solution = true;
            res.solution = sol;
            res.solution.resize(n_orig);

            Float64 obj = model.objective_offset();
            for (Index j = 0; j < n_orig; ++j) {
                obj += model.lp_model().get_variable(j).obj_coeff * res.solution[j];
            }
            res.objective_value = obj;
            return res;
        }

        // Find most fractional integer variable to fix/clamp
        Index best_var = -1;
        Float64 max_frac = 0.0;
        Float64 best_val = 0.0;

        for (Index j : integer_var_indices) {
            if (j < 0 || j >= static_cast<Index>(sol.size())) continue;
            Float64 val = sol[j];
            Float64 frac = std::abs(val - std::floor(val + 0.5));
            if (frac > tol_integrality_ && frac > max_frac) {
                max_frac = frac;
                best_var = j;
                best_val = val;
            }
        }

        if (best_var < 0) break;

        // Fractional diving bound fixing strategy: fix to nearest integer bound
        Float64 rounded = std::floor(best_val + 0.5);
        auto& var = dive_lp.get_variable(best_var);
        if (rounded >= best_val) {
            var.lower_bound = std::max(var.lower_bound, rounded);
        } else {
            var.upper_bound = std::min(var.upper_bound, rounded);
        }
    }

    return res;
}

} // namespace sovereign::milp
