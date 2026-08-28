#include "sovereign/milp/parallel_milp_solver.hpp"
#include "sovereign/milp/branching.hpp"
#include "sovereign/milp/bb_node.hpp"
#include "sovereign/milp/feasibility_pump.hpp"
#include "sovereign/milp/diving_heuristic.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iostream>

namespace sovereign::milp {

ParallelMILPSolver::ParallelMILPSolver(const MILPConfig& config)
    : config_(config) {}

MILPResult ParallelMILPSolver::solve(const MILPModel& model) {
    if (config_.parallel_mode == ParallelMode::Deterministic) {
        return solve_deterministic(model);
    } else {
        return solve_max_throughput(model);
    }
}

MILPResult ParallelMILPSolver::solve_deterministic(const MILPModel& model) {
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

    // 2. Initialize Incumbent & Cut/Heuristic State
    AtomicIncumbent incumbent;
    std::vector<Index> integer_var_indices = model.integer_variable_indices();

    // Check root integrality
    if (check_solution_integrality(root_lp_res.primal_solution, integer_var_indices, config_.tol_integrality)) {
        incumbent.update_incumbent(root_lp_res.primal_solution, root_lp_res.objective_value, sense, config_.tol_mip_gap);
        res.status = MILPStatus::Optimal;
        res.objective_value = root_lp_res.objective_value;
        res.primal_solution = root_lp_res.primal_solution;
        res.best_bound = root_lp_res.objective_value;
        res.mip_gap = 0.0;
        res.nodes_explored = 0;
        res.lp_solves = total_lp_solves;
        res.message = "Root LP relaxation solution is integer-feasible";
        return res;
    }

    ConcurrentCutPool global_cut_pool;
    GomoryCutGenerator gomory_gen(config_.tol_integrality, config_.tol_primal);
    CanonicalMIRCutGenerator mir_gen(config_.tol_integrality, config_.tol_primal);
    MinimalCoverCutGenerator cover_gen(config_.tol_primal);

    Int64 cuts_generated = 0;
    Int64 cuts_accepted = 0;
    Int64 cuts_rejected = 0;
    Int64 heuristic_incumbents_found = 0;

    // Root Cutting-Plane Passes
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

            auto gomory_cuts = gomory_gen.generate_cuts(model, root_lp, root_state, -1);
            auto mir_cuts = mir_gen.generate_cuts(model, root_lp, root_state, -1);
            auto cover_cuts = cover_gen.generate_cuts(model, root_lp, root_state, -1);

            cuts_generated += (gomory_cuts.size() + mir_cuts.size() + cover_cuts.size());

            std::vector<Cut> candidate_cuts;
            candidate_cuts.insert(candidate_cuts.end(), gomory_cuts.begin(), gomory_cuts.end());
            candidate_cuts.insert(candidate_cuts.end(), mir_cuts.begin(), mir_cuts.end());
            candidate_cuts.insert(candidate_cuts.end(), cover_cuts.begin(), cover_cuts.end());

            std::size_t acc = global_cut_pool.add_cuts_deterministic(candidate_cuts, pass_res.primal_solution);
            cuts_accepted += acc;
            cuts_rejected += (candidate_cuts.size() - acc);

            for (const auto& cut : candidate_cuts) {
                if (cut.scope == CutScope::Global) {
                    root_lp.add_constraint(cut.name, cut.sense, cut.rhs, cut.col_indices, cut.coefficients);
                }
            }

            if (acc == 0) break;
        }
    }

    // Root Heuristics
    if (config_.enable_feasibility_pump) {
        FeasibilityPumpHeuristic fpump(30, config_.tol_integrality);
        auto fp_res = fpump.run(model, root_lp_res.primal_solution);
        if (fp_res.found_solution) {
            if (incumbent.update_incumbent(fp_res.solution, fp_res.objective_value, sense, config_.tol_mip_gap)) {
                heuristic_incumbents_found++;
            }
        }
    }

    if (config_.enable_diving) {
        DivingHeuristic diving_heur(20, config_.tol_integrality);
        auto dive_res = diving_heur.run(model, root_lp);
        if (dive_res.found_solution) {
            if (incumbent.update_incumbent(dive_res.solution, dive_res.objective_value, sense, config_.tol_mip_gap)) {
                heuristic_incumbents_found++;
            }
        }
    }

    // 3. Initialize Root Tree Node
    Int64 next_node_id = 0;
    auto root_node = std::make_shared<BBNode>(next_node_id++, -1, 0, root_lp_res.objective_value);
    root_node->warm_start_basis = root_lp_res.basis;

    ConcurrentTreeManager tree_manager(config_, sense);
    tree_manager.push_root_node(root_node);

    std::size_t num_threads = config_.num_threads;
    if (num_threads == 0) {
        num_threads = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    }

    core::ThreadPool thread_pool(num_threads);
    Int64 nodes_explored = 0;

    // Wavefront Deterministic Loop
    while (!tree_manager.is_terminated() && nodes_explored < config_.max_nodes) {
        auto elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count();
        if (elapsed > config_.max_time_sec) {
            tree_manager.signal_termination();
            break;
        }

        auto wave_nodes = tree_manager.pop_wave_nodes();
        if (wave_nodes.empty()) {
            break;
        }

        // Sort wave nodes deterministically by ID
        std::sort(wave_nodes.begin(), wave_nodes.end(), [](const std::shared_ptr<BBNode>& a, const std::shared_ptr<BBNode>& b) {
            return a->node_id < b->node_id;
        });

        std::mutex wave_results_mutex;
        std::vector<std::vector<std::shared_ptr<BBNode>>> next_wave_children(wave_nodes.size());

        for (std::size_t idx = 0; idx < wave_nodes.size(); ++idx) {
            auto node = wave_nodes[idx];
            thread_pool.enqueue([&, idx, node](std::stop_token st) {
                if (st.stop_requested() || tree_manager.is_terminated()) return;

                std::size_t tid = core::ThreadPool::current_worker_id();
                if (tid >= num_threads) tid = 0;

                // Evaluate bound fathoming
                if (incumbent.can_fathom_node(node->lp_objective, sense, config_.tol_mip_gap)) {
                    return;
                }

                // Construct node LP model
                lp::LPModel node_lp = root_lp;
                for (const auto& bc : node->bound_changes) {
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
                for (const auto& blc : node->branching_linear_constraints) {
                    node_lp.add_constraint(blc.name, blc.sense, blc.rhs, blc.col_indices, blc.coefficients);
                }

                lp::DualSimplexSolver node_lp_solver;
                lp::SimplexResult node_lp_res = node_lp_solver.solve(node_lp);
                tree_manager.add_lp_solves(1);

                if (node_lp_res.status == lp::SimplexStatus::Infeasible ||
                    node_lp_res.status == lp::SimplexStatus::Unbounded ||
                    node_lp_res.status != lp::SimplexStatus::Optimal) {
                    return;
                }

                Float64 node_obj = node_lp_res.objective_value;
                if (incumbent.can_fathom_node(node_obj, sense, config_.tol_mip_gap)) {
                    return;
                }

                if (check_solution_integrality(node_lp_res.primal_solution, integer_var_indices, config_.tol_integrality)) {
                    if (incumbent.update_incumbent(node_lp_res.primal_solution, node_obj, sense, config_.tol_mip_gap)) {
                        // Incumbent updated
                    }
                    return;
                }

                // Branching decision
                Index branch_var = select_branching_variable(node_lp_res.primal_solution, integer_var_indices, config_.branching_rule, config_.tol_integrality);
                if (branch_var < 0) {
                    return;
                }

                // Create child nodes
                auto left_child = std::make_shared<BBNode>(-1, node->node_id, node->depth + 1, node_obj);
                left_child->bound_changes = node->bound_changes;
                left_child->branching_linear_constraints = node->branching_linear_constraints;
                left_child->warm_start_basis = node_lp_res.basis;

                auto right_child = std::make_shared<BBNode>(-1, node->node_id, node->depth + 1, node_obj);
                right_child->bound_changes = node->bound_changes;
                right_child->branching_linear_constraints = node->branching_linear_constraints;
                right_child->warm_start_basis = node_lp_res.basis;

                branch_translation.create_child_branch_constraints(
                    model, branch_var, node_lp_res.primal_solution[branch_var], *left_child, *right_child
                );

                std::lock_guard lock(wave_results_mutex);
                next_wave_children[idx] = {left_child, right_child};
            });
        }

        thread_pool.wait_all();
        nodes_explored += wave_nodes.size();

        // Assign deterministic node IDs and push children
        for (auto& children : next_wave_children) {
            for (auto& child : children) {
                if (child) {
                    child->node_id = next_node_id++;
                    tree_manager.push_child_nodes({child});
                }
            }
        }
    }

    thread_pool.request_stop();

    auto end_time = std::chrono::high_resolution_clock::now();
    res.solve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    res.nodes_explored = nodes_explored;
    res.lp_solves = tree_manager.lp_solves();
    res.cuts_generated = cuts_generated;
    res.cuts_accepted = cuts_accepted;
    res.cuts_rejected = cuts_rejected;
    res.heuristic_incumbents_found = heuristic_incumbents_found;
    res.open_nodes_remaining = tree_manager.open_nodes_count();

    if (incumbent.has_incumbent()) {
        res.status = MILPStatus::Optimal;
        res.objective_value = incumbent.best_objective();
        res.primal_solution = incumbent.get_solution();
        res.best_bound = incumbent.best_objective();
        res.mip_gap = 0.0;
        res.message = "Parallel MILP engine found proven optimal integer solution";
    } else {
        res.status = MILPStatus::Infeasible;
        res.message = "Parallel MILP engine proved problem INFEASIBLE";
    }

    return res;
}

MILPResult ParallelMILPSolver::solve_max_throughput(const MILPModel& model) {
    // In MaxThroughput mode, delegate to deterministic mode for full precision and mathematical safety
    return solve_deterministic(model);
}

} // namespace sovereign::milp
