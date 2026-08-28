#include "sovereign/milp/rins_heuristic.hpp"
#include "sovereign/milp/branching.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include <cmath>
#include <algorithm>

namespace sovereign::milp {

RINSHeuristic::RINSHeuristic(Int64 sub_node_limit, Float64 tol_integrality)
    : sub_node_limit_(sub_node_limit), tol_integrality_(tol_integrality) {}

HeuristicResult RINSHeuristic::run(
    const MILPModel& model,
    std::span<const Float64> incumbent_solution,
    std::span<const Float64> lp_solution
) const {
    HeuristicResult res;
    res.found_solution = false;

    Index n_orig = model.num_variables();
    if (n_orig == 0 || incumbent_solution.empty() || lp_solution.empty()) return res;

    std::vector<Index> integer_var_indices = model.integer_variable_indices();
    if (integer_var_indices.empty()) return res;

    // Create sub-MILP model from base model
    MILPModel sub_milp = model;

    // Fix integer variables where incumbent and continuous LP solution agree
    for (Index j : integer_var_indices) {
        if (j < 0 || j >= static_cast<Index>(incumbent_solution.size()) || j >= static_cast<Index>(lp_solution.size())) continue;

        Float64 inc_val = incumbent_solution[j];
        Float64 lp_val = lp_solution[j];

        if (std::abs(inc_val - lp_val) <= tol_integrality_) {
            Float64 fix_val = std::floor(inc_val + 0.5);
            auto& var = sub_milp.lp_model().get_variable(j);
            var.lower_bound = fix_val;
            var.upper_bound = fix_val;
        }
    }

    // Configure truncated B&B search for RINS sub-MILP
    MILPConfig sub_config;
    sub_config.max_nodes = sub_node_limit_;
    sub_config.enable_rins = false; // Prevent recursive RINS
    sub_config.enable_diving = false;

    MILPSolver sub_solver(sub_config);
    MILPResult sub_res = sub_solver.solve(sub_milp);

    if (sub_res.status == MILPStatus::Optimal && check_solution_integrality(sub_res.primal_solution, integer_var_indices, tol_integrality_)) {
        res.found_solution = true;
        res.solution = sub_res.primal_solution;
        res.solution.resize(n_orig);
        res.objective_value = sub_res.objective_value;
    }

    return res;
}

} // namespace sovereign::milp
