#include "sovereign/milp/milp_solver.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/milp/cut_pool.hpp"
#include "sovereign/milp/gomory_cut.hpp"
#include "sovereign/milp/mir_cut.hpp"
#include "sovereign/milp/cover_cut.hpp"
#include "sovereign/milp/feasibility_pump.hpp"
#include "sovereign/milp/diving_heuristic.hpp"
#include "sovereign/milp/rins_heuristic.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iostream>

#include "sovereign/milp/parallel_milp_solver.hpp"

namespace sovereign::milp {

MILPResult MILPSolver::solve(const MILPModel& model) const {
    if (config_.parallel_mode != ParallelMode::Serial && config_.num_threads != 1) {
        ParallelMILPSolver parallel_solver(config_);
        return parallel_solver.solve(model);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    MILPResult res;
    ObjectiveSense sense = model.objective_sense();

    Index n_orig = model.num_variables();
    if (n_orig == 0) {
        res.status = MILPStatus::Optimal;
        res.objective_value = model.objective_offset();
        res.best_bound = model.objective_offset();
        res.mip_gap = 0.0;
        res.nodes_explored = 0;
        res.lp_solves = 0;
        res.message = "Empty MILP model solved trivially";
        return res;
    }

    // 1. Root Continuous LP Relaxation
    lp::LPModel root_lp = model.lp_model();
    BranchTranslationLayer branch_translation;

    // Solve Root LP Relaxation
    lp::DualSimplexSolver root_lp_solver;
    auto root_lp_res = root_lp_solver.solve(root_lp);
    Int64 total_lp_solves = 1;

    if (root_lp_res.status == lp::SimplexStatus::Infeasible) {
        res.status = MILPStatus::Infeasible;
        res.message = "Root LP relaxation is INFEASIBLE (MILP is infeasible)";
        res.nodes_explored = 0;
        res.lp_solves = total_lp_solves;
        return res;
    }

    if (root_lp_res.status == lp::SimplexStatus::Unbounded) {
        res.status = MILPStatus::Unresolved;
        res.message = "Continuous LP relaxation is UNBOUNDED but no integer-feasible ray is established";
        res.nodes_explored = 0;
        res.lp_solves = total_lp_solves;
        return res;
    }

    if (root_lp_res.status != lp::SimplexStatus::Optimal) {
        res.status = MILPStatus::NumericalFailure;
        res.message = "Root LP relaxation failed with numerical status: " + root_lp_res.message;
        res.nodes_explored = 0;
        res.lp_solves = total_lp_solves;
        return res;
    }

    // 2. Initialize Branch-and-Bound & Cut/Heuristic State
    bool has_incumbent = false;
    Float64 incumbent_obj = (sense == ObjectiveSense::Minimize) ? INFINITY_VAL : -INFINITY_VAL;
    std::vector<Float64> incumbent_sol = root_lp_res.primal_solution;

    std::vector<Index> integer_var_indices = model.integer_variable_indices();

    CutPool global_cut_pool(config_.tol_primal, 0.90);
    GomoryCutGenerator gomory_gen(config_.tol_integrality, config_.tol_primal);
    CanonicalMIRCutGenerator mir_gen(config_.tol_integrality, config_.tol_primal);
    MinimalCoverCutGenerator cover_gen(config_.tol_primal);

    Int64 cuts_generated = 0;
    Int64 cuts_accepted = 0;
    Int64 cuts_rejected = 0;
    Int64 heuristic_incumbents_found = 0;

    // 3. Batch 8 Root Cutting-Plane Separation Loop
    if (config_.enable_cuts) {
        for (Int32 pass = 0; pass < config_.max_cut_passes; ++pass) {
            lp::DualSimplexSolver pass_solver;
            auto pass_res = pass_solver.solve(root_lp);
            if (pass_res.status != lp::SimplexStatus::Optimal) break;

            lp::LPModel std_pass_lp = root_lp.convert_to_standard_form();
            lp::SimplexState root_state(std_pass_lp, pass_res.basis);
            if (!is_ok(root_state.factorize_basis()) ||
                !is_ok(root_state.solve_primal_solution()) ||
                !is_ok(root_state.solve_dual_solution())) {
                break;
            }

            // Generate candidate cuts from all valid separators using current root_lp
            auto gomory_cuts = gomory_gen.generate_cuts(model, root_lp, root_state, -1);
            auto mir_cuts = mir_gen.generate_cuts(model, root_lp, root_state, -1);
            auto cover_cuts = cover_gen.generate_cuts(model, root_lp, root_state, -1);

            cuts_generated += (gomory_cuts.size() + mir_cuts.size() + cover_cuts.size());

            Int32 added_in_pass = 0;

            auto process_cuts = [&](std::vector<Cut>& cut_list) {
                for (auto& cut : cut_list) {
                    if (global_cut_pool.add_cut(cut, pass_res.primal_solution)) {
                        root_lp.add_constraint(cut.name, cut.sense, cut.rhs, cut.col_indices, cut.coefficients);
                        cuts_accepted++;
                        added_in_pass++;
                    } else {
                        cuts_rejected++;
                    }
                }
            };

            process_cuts(gomory_cuts);
            process_cuts(mir_cuts);
            process_cuts(cover_cuts);

            if (added_in_pass == 0) break; // Reached cut separation fixed point

            // Re-optimize root LP using Dual Simplex
            root_lp_res = pass_solver.solve(root_lp);
            total_lp_solves++;

            if (root_lp_res.status != lp::SimplexStatus::Optimal) break;
        }
    }

    // 4. Batch 8 Feasibility Pump Primal Heuristic at Root Node
    if (config_.enable_feasibility_pump) {
        FeasibilityPumpHeuristic fpump(30, config_.tol_integrality);
        auto fp_res = fpump.run(model, root_lp_res.primal_solution);
        if (fp_res.found_solution) {
            has_incumbent = true;
            incumbent_obj = fp_res.objective_value;
            incumbent_sol = fp_res.solution;
            heuristic_incumbents_found++;
        }
    }

    // Check if Root Relaxation is Integral
    bool root_is_integral = check_solution_integrality(root_lp_res.primal_solution, integer_var_indices, config_.tol_integrality);
    if (root_is_integral) {
        res.status = MILPStatus::Optimal;
        res.objective_value = root_lp_res.objective_value;
        res.primal_solution = root_lp_res.primal_solution;
        res.best_bound = root_lp_res.objective_value;
        res.mip_gap = 0.0;
        res.nodes_explored = 0;
        res.lp_solves = total_lp_solves;
        res.cuts_generated = cuts_generated;
        res.cuts_accepted = cuts_accepted;
        res.cuts_rejected = cuts_rejected;
        res.heuristic_incumbents_found = heuristic_incumbents_found;
        res.message = "Root continuous relaxation (with cuts) is integer-feasible";

        auto end_time = std::chrono::high_resolution_clock::now();
        res.solve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        return res;
    }

    // 5. Initialize B&B Tree Manager & Root Node
    BBTreeManager tree;
    BBNode root_node;
    root_node.node_id = tree.create_node_id();
    root_node.parent_id = -1;
    root_node.depth = 0;
    root_node.lp_objective = root_lp_res.objective_value;
    root_node.warm_start_basis = root_lp_res.basis;
    tree.push_node(root_node);

    Int64 nodes_explored = 0;

    // 6. Main Branch-and-Bound Loop
    while (!tree.empty() && nodes_explored < config_.max_nodes) {
        auto now_time = std::chrono::high_resolution_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now_time - start_time).count();
        if (elapsed_sec > config_.max_time_sec) {
            break; // Time limit reached
        }

        Float64 best_open_bound = tree.best_open_bound(sense);

        // Check MIP Gap & Prune Open Nodes if Incumbent Exists
        if (has_incumbent) {
            Float64 gap = std::abs(incumbent_obj - best_open_bound) / (1e-10 + std::abs(incumbent_obj));
            if (gap <= config_.tol_mip_gap) {
                break; // Global optimality gap reached
            }
            tree.prune_nodes_by_bound(incumbent_obj, config_.tol_mip_gap, sense);
            if (tree.empty()) break;
        }

        BBNode curr_node = tree.pop_node(config_.node_selection, sense);
        nodes_explored++;

        // Construct Child LP Relaxation Model
        lp::LPModel node_lp = root_lp;

        // Apply bound changes
        for (const auto& bc : curr_node.bound_changes) {
            if (bc.col_index >= 0 && bc.col_index < node_lp.num_variables()) {
                auto& var = node_lp.get_variable(bc.col_index);
                if (bc.new_lower_bound > -INFINITY_VAL / 2.0) {
                    var.lower_bound = std::max(var.lower_bound, bc.new_lower_bound);
                }
                if (bc.new_upper_bound < INFINITY_VAL / 2.0) {
                    var.upper_bound = std::min(var.upper_bound, bc.new_upper_bound);
                }
            }
        }

        // Apply branching linear constraints
        for (const auto& blc : curr_node.branching_linear_constraints) {
            node_lp.add_constraint(blc.name, blc.sense, blc.rhs, blc.col_indices, blc.coefficients);
        }

        // Solve Child Node LP cleanly
        lp::DualSimplexSolver node_solver;
        lp::SimplexResult node_lp_res = node_solver.solve(node_lp);
        total_lp_solves++;

        // --- Fathoming Checks ---
        // 1. Fathom by Infeasibility / Non-Optimal Status
        if (node_lp_res.status != lp::SimplexStatus::Optimal) {
            continue;
        }

        Float64 z_node = node_lp_res.objective_value;

        // 2. Fathom by Bound
        if (has_incumbent) {
            if (sense == ObjectiveSense::Minimize) {
                if (z_node > incumbent_obj + config_.tol_mip_gap) continue;
            } else {
                if (z_node < incumbent_obj - config_.tol_mip_gap) continue;
            }
        }

        const auto& node_sol = node_lp_res.primal_solution;

        // Batch 8 Fractional Diving Primal Heuristic during B&B Search
        if (config_.enable_diving && (nodes_explored % 5 == 0)) {
            DivingHeuristic diving(20, config_.tol_integrality);
            auto dive_res = diving.run(model, node_lp);
            if (dive_res.found_solution) {
                bool is_better = false;
                if (!has_incumbent) {
                    is_better = true;
                } else {
                    is_better = (sense == ObjectiveSense::Minimize) ? (dive_res.objective_value < incumbent_obj) : (dive_res.objective_value > incumbent_obj);
                }
                if (is_better) {
                    has_incumbent = true;
                    incumbent_obj = dive_res.objective_value;
                    incumbent_sol = dive_res.solution;
                    heuristic_incumbents_found++;
                    tree.prune_nodes_by_bound(incumbent_obj, config_.tol_mip_gap, sense);
                }
            }
        }

        // Batch 8 Optional RINS Heuristic
        if (config_.enable_rins && has_incumbent && (nodes_explored % 10 == 0)) {
            RINSHeuristic rins(30, config_.tol_integrality);
            auto rins_res = rins.run(model, incumbent_sol, node_sol);
            if (rins_res.found_solution) {
                bool is_better = (sense == ObjectiveSense::Minimize) ? (rins_res.objective_value < incumbent_obj) : (rins_res.objective_value > incumbent_obj);
                if (is_better) {
                    has_incumbent = true;
                    incumbent_obj = rins_res.objective_value;
                    incumbent_sol = rins_res.solution;
                    heuristic_incumbents_found++;
                    tree.prune_nodes_by_bound(incumbent_obj, config_.tol_mip_gap, sense);
                }
            }
        }

        // 3. Fathom by Integrality
        bool is_int = check_solution_integrality(node_sol, integer_var_indices, config_.tol_integrality);
        if (is_int) {
            if (!has_incumbent) {
                has_incumbent = true;
                incumbent_obj = z_node;
                incumbent_sol = node_sol;
            } else {
                bool is_better = (sense == ObjectiveSense::Minimize) ? (z_node < incumbent_obj) : (z_node > incumbent_obj);
                if (is_better) {
                    incumbent_obj = z_node;
                    incumbent_sol = node_sol;
                }
            }
            tree.prune_nodes_by_bound(incumbent_obj, config_.tol_mip_gap, sense);
            continue; // Fathomed by integrality
        }

        // --- Branching Step ---
        Index branch_var = select_branching_variable(node_sol, integer_var_indices, config_.branching_rule, config_.tol_integrality);
        if (branch_var < 0) continue;

        BBNode left_child;
        left_child.node_id = tree.create_node_id();
        left_child.parent_id = curr_node.node_id;
        left_child.depth = curr_node.depth + 1;
        left_child.lp_objective = z_node;
        left_child.warm_start_basis = node_lp_res.basis;
        left_child.bound_changes = curr_node.bound_changes;
        left_child.branching_linear_constraints = curr_node.branching_linear_constraints;

        BBNode right_child;
        right_child.node_id = tree.create_node_id();
        right_child.parent_id = curr_node.node_id;
        right_child.depth = curr_node.depth + 1;
        right_child.lp_objective = z_node;
        right_child.warm_start_basis = node_lp_res.basis;
        right_child.bound_changes = curr_node.bound_changes;
        right_child.branching_linear_constraints = curr_node.branching_linear_constraints;

        branch_translation.create_child_branch_constraints(
            model, branch_var, node_sol[branch_var], left_child, right_child
        );

        tree.push_node(std::move(left_child));
        tree.push_node(std::move(right_child));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    res.solve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    res.nodes_explored = nodes_explored;
    res.lp_solves = total_lp_solves;
    res.open_nodes_remaining = static_cast<Int64>(tree.open_node_count());

    res.cuts_generated = cuts_generated;
    res.cuts_accepted = cuts_accepted;
    res.cuts_rejected = cuts_rejected;
    res.heuristic_incumbents_found = heuristic_incumbents_found;

    if (has_incumbent) {
        res.status = MILPStatus::Optimal;
        res.objective_value = incumbent_obj;
        res.primal_solution = incumbent_sol;

        Float64 best_bound = tree.empty() ? incumbent_obj : tree.best_open_bound(sense);
        res.best_bound = best_bound;
        res.mip_gap = std::abs(incumbent_obj - best_bound) / (1e-10 + std::abs(incumbent_obj));
        res.message = "MILP solver converged to optimal integer solution";
    } else {
        res.status = MILPStatus::Infeasible;
        res.objective_value = 0.0;
        res.best_bound = (sense == ObjectiveSense::Minimize) ? INFINITY_VAL : -INFINITY_VAL;
        res.mip_gap = 1.0;
        res.message = "Branch-and-Bound tree exhausted with no integer feasible solution (Infeasible)";
    }

    return res;
}

} // namespace sovereign::milp
