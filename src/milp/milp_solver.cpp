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
#include <sstream>
#include <iomanip>

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

    auto get_time_ms = [&]() -> double {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_time).count();
    };

    auto emit_telemetry = [&](const std::string& json_line) {
        if (config_.telemetry_callback) {
            config_.telemetry_callback(json_line);
        }
    };

    auto emit_incumbent_found = [&](Int64 node_id, Float64 obj, const std::string& source) {
        std::ostringstream ss;
        ss << "{\"event\":\"incumbent_found\",\"timestamp_ms\":" << get_time_ms()
           << ",\"node_id\":" << node_id
           << ",\"objective\":" << std::fixed << std::setprecision(6) << obj
           << ",\"source\":\"" << source << "\""
           << ",\"time_sec\":" << std::fixed << std::setprecision(3) << (get_time_ms() / 1000.0) << "}";
        emit_telemetry(ss.str());
    };

    auto emit_bound_improved = [&](Float64 bound, Int64 node_id = -1) {
        std::ostringstream ss;
        ss << "{\"event\":\"bound_improved\",\"timestamp_ms\":" << get_time_ms()
           << ",\"best_bound\":" << std::fixed << std::setprecision(6) << bound
           << ",\"node_id\":" << node_id
           << ",\"time_sec\":" << std::fixed << std::setprecision(3) << (get_time_ms() / 1000.0) << "}";
        emit_telemetry(ss.str());
    };

    auto emit_node_created = [&](Int64 node_id, Int64 parent_id, Int32 depth, Float64 bound, Index branch_var = -1, const std::string& dir = "") {
        std::ostringstream ss;
        ss << "{\"event\":\"node_created\",\"timestamp_ms\":" << get_time_ms()
           << ",\"node_id\":" << node_id
           << ",\"parent_id\":" << parent_id
           << ",\"depth\":" << depth
           << ",\"bound\":" << std::fixed << std::setprecision(6) << bound;
        if (branch_var >= 0) {
            ss << ",\"branch_var\":" << branch_var
               << ",\"branch_direction\":\"" << dir << "\"";
        }
        ss << "}";
        emit_telemetry(ss.str());
    };

    auto emit_node_solved = [&](Int64 node_id, Int32 depth, int lp_status, Float64 lp_obj) {
        std::ostringstream ss;
        ss << "{\"event\":\"node_solved\",\"timestamp_ms\":" << get_time_ms()
           << ",\"node_id\":" << node_id
           << ",\"depth\":" << depth
           << ",\"lp_status\":" << lp_status
           << ",\"lp_objective\":" << std::fixed << std::setprecision(6) << lp_obj << "}";
        emit_telemetry(ss.str());
    };

    auto emit_node_fathomed = [&](Int64 node_id, Int64 parent_id, Int32 depth, const std::string& reason, Float64 bound) {
        std::ostringstream ss;
        ss << "{\"event\":\"node_fathomed\",\"timestamp_ms\":" << get_time_ms()
           << ",\"node_id\":" << node_id
           << ",\"parent_id\":" << parent_id
           << ",\"depth\":" << depth
           << ",\"reason\":\"" << reason << "\""
           << ",\"bound\":" << std::fixed << std::setprecision(6) << bound << "}";
        emit_telemetry(ss.str());
    };

    // 0. Emit Solve Started Event
    {
        std::ostringstream ss;
        ss << "{\"event\":\"solve_started\",\"timestamp_ms\":" << get_time_ms()
           << ",\"model_name\":\"" << model.lp_model().name() << "\""
           << ",\"sense\":\"" << (sense == ObjectiveSense::Minimize ? "MINIMIZE" : "MAXIMIZE") << "\""
           << ",\"variables\":" << model.num_variables()
           << ",\"constraints\":" << model.num_constraints() << "}";
        emit_telemetry(ss.str());
    }

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
    SolverConfig lp_cfg;
    lp_cfg.max_time_sec = config_.max_time_sec;
    lp_cfg.max_iterations = 20000;
    lp::DualSimplexSolver root_lp_solver(lp_cfg);
    auto root_lp_res = root_lp_solver.solve(root_lp);
    Int64 total_lp_solves = 1;

    Index frac_bin_cnt = 0;
    for (Index j = 0; j < model.num_variables(); ++j) {
        if (model.is_integer(j) && !root_lp_res.primal_solution.empty()) {
            Float64 val = root_lp_res.primal_solution[j];
            if (std::abs(val - std::round(val)) > 1e-6) frac_bin_cnt++;
        }
    }

    // Emit Root LP Event
    {
        std::ostringstream ss;
        ss << "{\"event\":\"root_lp\",\"timestamp_ms\":" << get_time_ms()
           << ",\"root_lp_status\":" << static_cast<int>(root_lp_res.status)
           << ",\"root_lp_objective\":" << std::fixed << std::setprecision(6) << root_lp_res.objective_value
           << ",\"fractional_binaries\":" << frac_bin_cnt
           << ",\"root_lp_iterations\":" << root_lp_res.iterations << "}";
        emit_telemetry(ss.str());
    }

    if (root_lp_res.status == lp::SimplexStatus::TimeLimit) {
        res.status = MILPStatus::TimeLimit;
        res.message = "Root LP relaxation reached time limit (" + std::to_string(config_.max_time_sec) + "s)";
        res.nodes_explored = 0;
        res.lp_solves = total_lp_solves;
        return res;
    }

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
            SolverConfig pass_cfg;
            pass_cfg.max_time_sec = config_.max_time_sec;
            pass_cfg.max_iterations = 20000;
            lp::DualSimplexSolver pass_solver(pass_cfg);
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
            auto new_root_res = pass_solver.solve(root_lp);
            total_lp_solves++;

            if (new_root_res.status == lp::SimplexStatus::Optimal) {
                root_lp_res = new_root_res;
            } else {
                break; // Stop cut passes if re-optimization fails or becomes infeasible
            }
        }
    }

    // 4. Batch 8 Feasibility Pump & Diving Primal Heuristics at Root Node
    if (config_.enable_feasibility_pump) {
        FeasibilityPumpHeuristic fpump(30, config_.tol_integrality);
        auto fp_res = fpump.run(model, root_lp_res.primal_solution);
        if (fp_res.found_solution) {
            has_incumbent = true;
            incumbent_obj = fp_res.objective_value;
            incumbent_sol = fp_res.solution;
            heuristic_incumbents_found++;
            emit_incumbent_found(-1, incumbent_obj, "FeasibilityPump");
        }
    }

    if (config_.enable_diving) {
        DivingHeuristic diver(100, config_.tol_integrality);
        auto dive_res = diver.run(model, root_lp);
        if (dive_res.found_solution) {
            if (!has_incumbent || (sense == ObjectiveSense::Minimize ? dive_res.objective_value < incumbent_obj : dive_res.objective_value > incumbent_obj)) {
                has_incumbent = true;
                incumbent_obj = dive_res.objective_value;
                incumbent_sol = dive_res.solution;
                heuristic_incumbents_found++;
                emit_incumbent_found(-1, incumbent_obj, "DivingHeuristic");
            }
        }
    }

    // Check if Root Relaxation is Integral
    bool root_is_integral = check_solution_integrality(root_lp_res.primal_solution, integer_var_indices, config_.tol_integrality);
    if (root_is_integral) {
        emit_incumbent_found(0, root_lp_res.objective_value, "RootLP");
        emit_bound_improved(root_lp_res.objective_value, 0);

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

        {
            std::ostringstream ss;
            ss << "{\"event\":\"solve_finished\",\"timestamp_ms\":" << get_time_ms()
               << ",\"status_name\":\"Optimal\""
               << ",\"objective_value\":" << std::fixed << std::setprecision(6) << res.objective_value
               << ",\"best_bound\":" << std::fixed << std::setprecision(6) << res.best_bound
               << ",\"mip_gap\":0.000000"
               << ",\"nodes_explored\":0,\"nodes_created\":0,\"lp_solves\":" << total_lp_solves << "}";
            emit_telemetry(ss.str());
        }

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

    emit_node_created(root_node.node_id, root_node.parent_id, root_node.depth, root_node.lp_objective);
    emit_bound_improved(root_lp_res.objective_value, root_node.node_id);

    Int64 nodes_explored = 0;
    Int64 nodes_created = 1;
    Int64 nodes_pruned_infeasible = 0;
    Int64 nodes_pruned_bound = 0;
    Int64 nodes_pruned_integrality = 0;
    Int64 nodes_unresolved = 0;

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
        emit_bound_improved(best_open_bound, curr_node.node_id);

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

        // Solve Child Node LP cleanly with Level-A warm-start if available
        SolverConfig node_cfg;
        node_cfg.max_time_sec = config_.max_time_sec;
        node_cfg.max_iterations = 20000;
        lp::DualSimplexSolver node_solver(node_cfg);
        
        lp::SimplexResult node_lp_res;
        if (curr_node.warm_start_basis.num_rows() > 0 && curr_node.warm_start_basis.num_cols() > 0) {
            node_lp_res = node_solver.solve(node_lp, curr_node.warm_start_basis);
        } else {
            node_lp_res = node_solver.solve(node_lp);
        }
        total_lp_solves++;

        emit_node_solved(curr_node.node_id, curr_node.depth, static_cast<int>(node_lp_res.status), node_lp_res.objective_value);

        // --- Explicit LP Status Fathoming Classification ---
        if (node_lp_res.status == lp::SimplexStatus::Infeasible) {
            nodes_pruned_infeasible++;
            emit_node_fathomed(curr_node.node_id, curr_node.parent_id, curr_node.depth, "infeasible", node_lp_res.objective_value);
            continue; // Safely fathomed by proven LP infeasibility
        }

        if (node_lp_res.status != lp::SimplexStatus::Optimal) {
            // IterationLimit, NumericalFailure, Unbounded, etc. are NOT infeasible!
            nodes_unresolved++;
            emit_node_fathomed(curr_node.node_id, curr_node.parent_id, curr_node.depth, "unresolved", node_lp_res.objective_value);
            continue; // Node cannot be fathomed nor branched upon
        }

        Float64 z_node = node_lp_res.objective_value;

        // 2. Fathom by Bound (Only when LP status is Optimal)
        if (has_incumbent) {
            bool prune = (sense == ObjectiveSense::Minimize) ? 
                         (z_node > incumbent_obj + config_.tol_mip_gap) : 
                         (z_node < incumbent_obj - config_.tol_mip_gap);
            if (prune) {
                nodes_pruned_bound++;
                emit_node_fathomed(curr_node.node_id, curr_node.parent_id, curr_node.depth, "bound_pruned", z_node);
                continue;
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
                    emit_incumbent_found(curr_node.node_id, incumbent_obj, "NodeDivingHeuristic");
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
                    emit_incumbent_found(curr_node.node_id, incumbent_obj, "RINSHeuristic");
                    tree.prune_nodes_by_bound(incumbent_obj, config_.tol_mip_gap, sense);
                }
            }
        }

        // 3. Fathom by Integrality
        bool is_int = check_solution_integrality(node_sol, integer_var_indices, config_.tol_integrality);
        if (is_int) {
            nodes_pruned_integrality++;
            if (!has_incumbent) {
                has_incumbent = true;
                incumbent_obj = z_node;
                incumbent_sol = node_sol;
                emit_incumbent_found(curr_node.node_id, incumbent_obj, "IntegerSolution");
            } else {
                bool is_better = (sense == ObjectiveSense::Minimize) ? (z_node < incumbent_obj) : (z_node > incumbent_obj);
                if (is_better) {
                    incumbent_obj = z_node;
                    incumbent_sol = node_sol;
                    emit_incumbent_found(curr_node.node_id, incumbent_obj, "IntegerSolution");
                }
            }
            emit_node_fathomed(curr_node.node_id, curr_node.parent_id, curr_node.depth, "integer_feasible", z_node);
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

        emit_node_created(left_child.node_id, left_child.parent_id, left_child.depth, left_child.lp_objective, branch_var, "LOWER");
        tree.push_node(std::move(left_child));

        emit_node_created(right_child.node_id, right_child.parent_id, right_child.depth, right_child.lp_objective, branch_var, "UPPER");
        tree.push_node(std::move(right_child));
        nodes_created += 2;
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

    res.nodes_created = nodes_created;
    res.nodes_pruned_infeasible = nodes_pruned_infeasible;
    res.nodes_pruned_bound = nodes_pruned_bound;
    res.nodes_pruned_integrality = nodes_pruned_integrality;
    res.nodes_unresolved = nodes_unresolved;

    // --- Strict Final Status Semantics ---
    bool queue_empty = tree.empty();
    bool all_nodes_proven = queue_empty && (nodes_unresolved == 0);

    if (has_incumbent) {
        res.objective_value = incumbent_obj;
        res.primal_solution = incumbent_sol;

        Float64 best_bound = queue_empty ? incumbent_obj : tree.best_open_bound(sense);
        res.best_bound = best_bound;
        res.mip_gap = std::abs(incumbent_obj - best_bound) / (1e-10 + std::abs(incumbent_obj));

        if (all_nodes_proven || res.mip_gap <= config_.tol_mip_gap) {
            res.status = MILPStatus::Optimal;
            res.message = "MILP solver converged to optimal integer solution";
        } else if (res.solve_time_ms / 1000.0 >= config_.max_time_sec) {
            res.status = MILPStatus::TimeLimit;
            res.message = "Time limit reached with feasible integer solution";
        } else if (nodes_explored >= config_.max_nodes) {
            res.status = MILPStatus::NodeLimit;
            res.message = "Node limit reached with feasible integer solution";
        } else {
            res.status = MILPStatus::Unresolved;
            res.message = "Search finished with unresolved nodes remaining";
        }
    } else {
        if (all_nodes_proven) {
            res.status = MILPStatus::Infeasible;
            res.objective_value = 0.0;
            res.best_bound = (sense == ObjectiveSense::Minimize) ? INFINITY_VAL : -INFINITY_VAL;
            res.mip_gap = 1.0;
            res.message = "Branch-and-Bound tree exhausted with all branches proven infeasible (Infeasible)";
        } else if (res.solve_time_ms / 1000.0 >= config_.max_time_sec) {
            res.status = MILPStatus::TimeLimit;
            res.message = "Time limit reached with no integer solution found";
        } else if (nodes_explored >= config_.max_nodes) {
            res.status = MILPStatus::NodeLimit;
            res.message = "Node limit reached with no integer solution found";
        } else {
            res.status = MILPStatus::Unresolved;
            res.objective_value = 0.0;
            res.best_bound = root_lp_res.objective_value; // Root bound preserved
            res.mip_gap = 1.0;
            res.message = "Search terminated with unresolved/interrupted nodes; integer status unproven";
        }
    }

    {
        std::string st_str = "Unknown";
        switch (res.status) {
            case MILPStatus::Optimal: st_str = "Optimal"; break;
            case MILPStatus::Infeasible: st_str = "Infeasible"; break;
            case MILPStatus::Unbounded: st_str = "Unbounded"; break;
            case MILPStatus::TimeLimit: st_str = "TimeLimit"; break;
            case MILPStatus::NodeLimit: st_str = "NodeLimit"; break;
            case MILPStatus::NumericalFailure: st_str = "NumericalFailure"; break;
            case MILPStatus::Unresolved: st_str = "Unresolved"; break;
        }
        std::ostringstream ss;
        ss << "{\"event\":\"solve_finished\",\"timestamp_ms\":" << get_time_ms()
           << ",\"status_name\":\"" << st_str << "\""
           << ",\"objective_value\":" << std::fixed << std::setprecision(6) << res.objective_value
           << ",\"best_bound\":" << std::fixed << std::setprecision(6) << res.best_bound
           << ",\"mip_gap\":" << std::fixed << std::setprecision(6) << res.mip_gap
           << ",\"nodes_explored\":" << res.nodes_explored
           << ",\"nodes_created\":" << res.nodes_created
           << ",\"lp_solves\":" << res.lp_solves << "}";
        emit_telemetry(ss.str());
    }

    return res;
}

} // namespace sovereign::milp
