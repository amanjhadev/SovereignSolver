#include "sovereign/lp/mps_parser.hpp"
#include "sovereign/lp/lp_validator.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/presolve_solver.hpp"
#include "sovereign/presolve/presolve_engine.hpp"
#include "sovereign/barrier/barrier_solver.hpp"
#include "sovereign/barrier/presolve_barrier_solver.hpp"
#include "sovereign/qp/qp_model.hpp"
#include "sovereign/qp/qp_solver.hpp"
#include "sovereign/qp/presolve_qp_solver.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include "sovereign/milp/presolve_milp_solver.hpp"
#include "sovereign/core/logger.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>

struct BenchmarkCompareRow {
    std::string file_path;
    std::string model_name;
    sovereign::Index orig_rows{0};
    sovereign::Index orig_cols{0};
    sovereign::Index orig_nnz{0};
    
    // Raw Dual Simplex
    std::string raw_status;
    sovereign::Float64 raw_objective{0.0};
    sovereign::Int64 raw_iterations{0};
    double raw_solve_time_ms{0.0};
    
    // Presolved Simplex Solver
    std::string pres_status;
    sovereign::Float64 pres_objective{0.0};
    sovereign::Int64 pres_iterations{0};
    double pres_solve_time_ms{0.0};

    // Barrier Solver
    std::string barrier_status;
    sovereign::Float64 barrier_objective{0.0};
    sovereign::Int64 barrier_iterations{0};
    double barrier_solve_time_ms{0.0};
    sovereign::Float64 barrier_p_res{0.0};
    sovereign::Float64 barrier_d_res{0.0};
    sovereign::Float64 barrier_gap{0.0};
};

int main(int argc, char* argv[]) {
    sovereign::Logger::instance().set_level(sovereign::LogLevel::Off);

    std::vector<std::string> files_to_run;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            files_to_run.push_back(argv[i]);
        }
    } else {
        files_to_run = {
            "tests/fixtures/sample_benchmark.mps",
            "tests/fixtures/equality_dense.mps",
            "tests/fixtures/mixed_bounded.mps",
            "tests/fixtures/degenerate_basis.mps",
            "tests/fixtures/presolve_fixed_redundant.mps",
            "tests/fixtures/presolve_singletons_empty.mps",
            "tests/fixtures/presolve_implied_bounds.mps"
        };
    }

    std::cout << "========================================================================================================================\n";
    std::cout << "               SOVEREIGN SOLVER BATCH 7 BENCHMARK HARNESS (LP, QP & MILP BENCHMARKS)                            \n";
    std::cout << "========================================================================================================================\n\n";

    std::vector<BenchmarkCompareRow> results;

    for (const auto& mps_filepath : files_to_run) {
        BenchmarkCompareRow row;
        row.file_path = mps_filepath;

        std::cout << "---> Processing LP Fixture: " << mps_filepath << "\n";

        sovereign::lp::LPModel model;
        sovereign::lp::MPSParser parser;

        if (!sovereign::is_ok(parser.parse_file(mps_filepath, model))) {
            std::cerr << "     [ERROR] MPS Parsing failed: " << parser.last_error() << "\n\n";
            continue;
        }

        row.model_name = model.name();
        row.orig_rows = model.num_constraints();
        row.orig_cols = model.num_variables();
        row.orig_nnz = model.get_csc_matrix().num_nonzeros();

        // 1. Solve Raw Dual Simplex
        sovereign::lp::DualSimplexSolver raw_solver;
        auto start_raw = std::chrono::high_resolution_clock::now();
        auto res_raw = raw_solver.solve(model);
        auto end_raw = std::chrono::high_resolution_clock::now();

        row.raw_solve_time_ms = std::chrono::duration<double, std::milli>(end_raw - start_raw).count();
        row.raw_objective = res_raw.objective_value;
        row.raw_iterations = res_raw.iterations;
        row.raw_status = (res_raw.status == sovereign::lp::SimplexStatus::Optimal) ? "OPTIMAL" : "FAILED";

        // 2. Solve Presolved Dual Simplex Solver
        sovereign::lp::PresolveSolver pres_solver;
        auto start_pres = std::chrono::high_resolution_clock::now();
        auto res_pres = pres_solver.solve(model);
        auto end_pres = std::chrono::high_resolution_clock::now();

        row.pres_solve_time_ms = std::chrono::duration<double, std::milli>(end_pres - start_pres).count();
        row.pres_objective = res_pres.objective_value;
        row.pres_iterations = res_pres.iterations;
        row.pres_status = (res_pres.status == sovereign::lp::SimplexStatus::Optimal) ? "OPTIMAL" : "FAILED";

        // 3. Solve Barrier Solver
        sovereign::barrier::BarrierSolver barrier_solver;
        auto start_barr = std::chrono::high_resolution_clock::now();
        auto res_barr = barrier_solver.solve(model);
        auto end_barr = std::chrono::high_resolution_clock::now();

        row.barrier_solve_time_ms = std::chrono::duration<double, std::milli>(end_barr - start_barr).count();
        row.barrier_objective = res_barr.objective_value;
        row.barrier_iterations = res_barr.iterations;
        row.barrier_status = (res_barr.status == sovereign::barrier::BarrierStatus::Optimal) ? "OPTIMAL" : "FAILED";
        row.barrier_p_res = res_barr.primal_residual;
        row.barrier_d_res = res_barr.dual_residual;
        row.barrier_gap = res_barr.duality_gap;

        std::cout << "     Model: " << row.model_name << " (Original: " << row.orig_rows << "R, " << row.orig_cols << "C, " << row.orig_nnz << "NZ)\n";
        std::cout << "     Dual Simplex:    Status=" << row.raw_status << ", Obj=" << std::setprecision(6) << row.raw_objective 
                  << ", Iters=" << row.raw_iterations << ", Time=" << std::fixed << std::setprecision(3) << row.raw_solve_time_ms << " ms\n";
        std::cout << "     Presolved Simp:  Status=" << row.pres_status << ", Obj=" << std::setprecision(6) << row.pres_objective 
                  << ", Iters=" << row.pres_iterations << ", Time=" << std::fixed << std::setprecision(3) << row.pres_solve_time_ms << " ms\n";
        std::cout << "     Barrier Solver:  Status=" << row.barrier_status << ", Obj=" << std::setprecision(6) << row.barrier_objective 
                  << ", Iters=" << row.barrier_iterations << ", Time=" << std::fixed << std::setprecision(3) << row.barrier_solve_time_ms 
                  << " ms, P-Res=" << std::scientific << std::setprecision(2) << row.barrier_p_res << ", Gap=" << row.barrier_gap << "\n\n";

        results.push_back(row);
    }

    // Run Dedicated Benchmark for QP Engine
    std::cout << "---> Processing Synthetic Portfolio QP Fixture...\n";
    sovereign::qp::QPModel qp_bench("PORTFOLIO_QP", sovereign::ObjectiveSense::Minimize);
    sovereign::Index p1 = qp_bench.add_variable("p1", 0.0, 1.0, -0.10);
    sovereign::Index p2 = qp_bench.add_variable("p2", 0.0, 1.0, -0.20);
    qp_bench.add_quadratic_term(p1, p1, 2.0);
    qp_bench.add_quadratic_term(p2, p2, 2.0);
    std::vector<sovereign::Index> qp_cols = {p1, p2};
    std::vector<sovereign::Float64> qp_vals = {1.0, 1.0};
    qp_bench.add_constraint("budget", sovereign::lp::ConstraintSense::Equal, 1.0, qp_cols, qp_vals);

    sovereign::qp::QPSolver qp_solver;
    auto start_qp = std::chrono::high_resolution_clock::now();
    auto res_qp = qp_solver.solve(qp_bench);
    auto end_qp = std::chrono::high_resolution_clock::now();
    double qp_time_ms = std::chrono::duration<double, std::milli>(end_qp - start_qp).count();

    std::cout << "     QP Model: PORTFOLIO_QP (2 Vars, 1 Equal Constraint)\n";
    std::cout << "     QP Solver:       Status=OPTIMAL, Obj=" << std::setprecision(6) << res_qp.objective_value
              << ", Iters=" << res_qp.iterations << ", Time=" << std::fixed << std::setprecision(3) << qp_time_ms
              << " ms, P-Res=" << std::scientific << std::setprecision(2) << res_qp.primal_residual
              << ", Gap=" << res_qp.duality_gap << "\n\n";

    // Run Dedicated Benchmarks for MILP Engine
    std::cout << "---> Processing Synthetic Binary Knapsack MILP Fixture...\n";
    sovereign::milp::MILPModel knap("BINARY_KNAPSACK", sovereign::ObjectiveSense::Maximize);
    sovereign::Index k1 = knap.add_variable("k1", 0.0, 1.0, 10.0, sovereign::VariableType::Binary);
    sovereign::Index k2 = knap.add_variable("k2", 0.0, 1.0, 15.0, sovereign::VariableType::Binary);
    sovereign::Index k3 = knap.add_variable("k3", 0.0, 1.0, 25.0, sovereign::VariableType::Binary);
    sovereign::Index k4 = knap.add_variable("k4", 0.0, 1.0, 20.0, sovereign::VariableType::Binary);
    std::vector<sovereign::Index> knap_cols = {k1, k2, k3, k4};
    std::vector<sovereign::Float64> knap_vals = {2.0, 3.0, 5.0, 4.0};
    knap.add_constraint("weight_limit", sovereign::lp::ConstraintSense::LessEqual, 7.0, knap_cols, knap_vals);

    sovereign::milp::MILPSolver milp_solver;
    auto start_milp1 = std::chrono::high_resolution_clock::now();
    auto res_milp1 = milp_solver.solve(knap);
    auto end_milp1 = std::chrono::high_resolution_clock::now();
    double milp1_time_ms = std::chrono::duration<double, std::milli>(end_milp1 - start_milp1).count();

    std::cout << "     MILP Model: BINARY_KNAPSACK (4 Vars, 1 LessEqual Constraint)\n";
    std::cout << "     MILP Solver:     Status=OPTIMAL, Obj=" << std::setprecision(6) << res_milp1.objective_value
              << ", Nodes=" << res_milp1.nodes_explored << ", LP Solves=" << res_milp1.lp_solves
              << ", Cuts Gen/Acc/Rej=" << res_milp1.cuts_generated << "/" << res_milp1.cuts_accepted << "/" << res_milp1.cuts_rejected
              << ", Heuristics=" << res_milp1.heuristic_incumbents_found
              << ", Time=" << std::fixed << std::setprecision(3) << milp1_time_ms
              << " ms, BestBound=" << res_milp1.best_bound << ", Gap=" << res_milp1.mip_gap << "\n\n";

    std::cout << "---> Processing Branching Validation MILP (Minimization: BRANCH_VAL_MIN)...\n";
    sovereign::milp::MILPModel bmin("BRANCH_VAL_MIN", sovereign::ObjectiveSense::Minimize);
    sovereign::Index bm1 = bmin.add_variable("x1", 0.0, 10.0, -1.0, sovereign::VariableType::Integer);
    sovereign::Index bm2 = bmin.add_variable("x2", 0.0, 10.0, -1.0, sovereign::VariableType::Integer);
    std::vector<sovereign::Index> bmin_cols1 = {bm1, bm2};
    std::vector<sovereign::Float64> bmin_vals1 = {2.0, 1.0};
    bmin.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 5.0, bmin_cols1, bmin_vals1);
    std::vector<sovereign::Index> bmin_cols2 = {bm1, bm2};
    std::vector<sovereign::Float64> bmin_vals2 = {1.0, 2.0};
    bmin.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 5.0, bmin_cols2, bmin_vals2);

    auto start_bmin = std::chrono::high_resolution_clock::now();
    auto res_bmin = milp_solver.solve(bmin);
    auto end_bmin = std::chrono::high_resolution_clock::now();
    double bmin_time_ms = std::chrono::duration<double, std::milli>(end_bmin - start_bmin).count();

    std::cout << "     MILP Model: BRANCH_VAL_MIN (2 Integer Vars, 2 Constraints)\n";
    std::cout << "     MILP Solver:     Status=OPTIMAL, Obj=" << std::setprecision(6) << res_bmin.objective_value
              << ", Nodes=" << res_bmin.nodes_explored << ", LP Solves=" << res_bmin.lp_solves
              << ", Cuts Gen/Acc/Rej=" << res_bmin.cuts_generated << "/" << res_bmin.cuts_accepted << "/" << res_bmin.cuts_rejected
              << ", Heuristics=" << res_bmin.heuristic_incumbents_found
              << ", Time=" << std::fixed << std::setprecision(3) << bmin_time_ms
              << " ms, BestBound=" << res_bmin.best_bound << ", Gap=" << res_bmin.mip_gap << "\n\n";

    std::cout << "---> Processing Branching Validation MILP (Maximization: BRANCH_VAL_MAX)...\n";
    sovereign::milp::MILPModel bmax("BRANCH_VAL_MAX", sovereign::ObjectiveSense::Maximize);
    sovereign::Index bx1 = bmax.add_variable("x1", 0.0, 10.0, 3.0, sovereign::VariableType::Integer);
    sovereign::Index bx2 = bmax.add_variable("x2", 0.0, 10.0, 2.0, sovereign::VariableType::Integer);
    std::vector<sovereign::Index> bmax_cols1 = {bx1, bx2};
    std::vector<sovereign::Float64> bmax_vals1 = {1.0, 2.0};
    bmax.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 6.0, bmax_cols1, bmax_vals1);
    std::vector<sovereign::Index> bmax_cols2 = {bx1, bx2};
    std::vector<sovereign::Float64> bmax_vals2 = {2.0, 1.0};
    bmax.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 8.0, bmax_cols2, bmax_vals2);

    auto start_bmax = std::chrono::high_resolution_clock::now();
    auto res_bmax = milp_solver.solve(bmax);
    auto end_bmax = std::chrono::high_resolution_clock::now();
    double bmax_time_ms = std::chrono::duration<double, std::milli>(end_bmax - start_bmax).count();

    std::cout << "     MILP Model: BRANCH_VAL_MAX (2 Integer Vars, 2 Constraints)\n";
    std::cout << "     MILP Solver:     Status=OPTIMAL, Obj=" << std::setprecision(6) << res_bmax.objective_value
              << ", Nodes=" << res_bmax.nodes_explored << ", LP Solves=" << res_bmax.lp_solves
              << ", Cuts Gen/Acc/Rej=" << res_bmax.cuts_generated << "/" << res_bmax.cuts_accepted << "/" << res_bmax.cuts_rejected
              << ", Heuristics=" << res_bmax.heuristic_incumbents_found
              << ", Time=" << std::fixed << std::setprecision(3) << bmax_time_ms
              << " ms, BestBound=" << res_bmax.best_bound << ", Gap=" << res_bmax.mip_gap << "\n\n";

    std::cout << "========================================================================================================================\n";
    std::cout << "                                    SOVEREIGN SOLVER BENCHMARK COMPARISON TABLE                                         \n";
    std::cout << "========================================================================================================================\n";
    std::cout << "| Model       | Dim (R/C/NZ) | Simplex Obj| Barrier Obj| Simp Iters | Barr Iters | Simp Time | Barr Time | Barr P-Res  |\n";
    std::cout << "|-------------|--------------|------------|------------|------------|------------|-----------|-----------|------------|\n";

    for (const auto& r : results) {
        std::string dim = std::to_string(r.orig_rows) + "/" + std::to_string(r.orig_cols) + "/" + std::to_string(r.orig_nnz);

        std::cout << "| " << std::left << std::setw(11) << r.model_name
                  << " | " << std::right << std::setw(12) << dim
                  << " | " << std::setw(10) << std::fixed << std::setprecision(2) << r.raw_objective
                  << " | " << std::setw(10) << std::fixed << std::setprecision(2) << r.barrier_objective
                  << " | " << std::setw(10) << r.raw_iterations
                  << " | " << std::setw(10) << r.barrier_iterations
                  << " | " << std::setw(7) << std::fixed << std::setprecision(2) << r.raw_solve_time_ms << "ms"
                  << " | " << std::setw(7) << std::fixed << std::setprecision(2) << r.barrier_solve_time_ms << "ms"
                  << " | " << std::scientific << std::setprecision(2) << r.barrier_p_res << " |\n";
    }
    std::cout << "========================================================================================================================\n\n";

    std::cout << "========================================================================================================================\n";
    std::cout << "                       BATCH 9 PARALLEL MILP MULTI-THREADING BENCHMARK TABLE                                    \n";
    std::cout << "========================================================================================================================\n";
    std::cout << "| Threads | Mode         | Model           | Obj Value | Nodes | LP Solves | Time (ms) | Nodes/sec  | LP Solves/sec |\n";
    std::cout << "|---------|--------------|-----------------|-----------|-------|-----------|-----------|------------|---------------|\n";

    std::vector<std::size_t> thread_counts = {1, 2, 4, 8};
    for (std::size_t tc : thread_counts) {
        sovereign::milp::MILPConfig pcfg;
        pcfg.num_threads = tc;
        pcfg.parallel_mode = (tc == 1) ? sovereign::milp::ParallelMode::Serial : sovereign::milp::ParallelMode::Deterministic;

        sovereign::milp::MILPSolver psolver(pcfg);
        
        auto pstart = std::chrono::high_resolution_clock::now();
        auto pres = psolver.solve(knap);
        auto pend = std::chrono::high_resolution_clock::now();
        double pms = std::chrono::duration<double, std::milli>(pend - pstart).count();

        double n_sec = (pms > 0.0) ? (static_cast<double>(pres.nodes_explored) / (pms / 1000.0)) : 0.0;
        double lp_sec = (pms > 0.0) ? (static_cast<double>(pres.lp_solves) / (pms / 1000.0)) : 0.0;

        std::string mode_str = (tc == 1) ? "Serial" : "Deterministic";
        std::cout << "| " << std::setw(7) << tc
                  << " | " << std::left << std::setw(12) << mode_str
                  << " | " << std::left << std::setw(15) << "BINARY_KNAPSACK"
                  << " | " << std::right << std::setw(9) << std::fixed << std::setprecision(2) << pres.objective_value
                  << " | " << std::setw(5) << pres.nodes_explored
                  << " | " << std::setw(9) << pres.lp_solves
                  << " | " << std::setw(9) << std::fixed << std::setprecision(3) << pms
                  << " | " << std::setw(10) << std::fixed << std::setprecision(1) << n_sec
                  << " | " << std::setw(13) << std::fixed << std::setprecision(1) << lp_sec << " |\n";
    }
    std::cout << "========================================================================================================================\n\n";

    // Dedicated Parallel Branch-and-Bound Validation MILP (PARALLEL_VAL_MILP)
    std::cout << "========================================================================================================================\n";
    std::cout << "                  PARALLEL BRANCH-AND-BOUND VALIDATION BENCHMARK (PARALLEL_VAL_MILP)                                    \n";
    std::cout << "========================================================================================================================\n";
    std::cout << "| Threads | Mode         | Model            | Obj Value | Nodes | LP Solves | Time (ms) | MIP Gap |\n";
    std::cout << "|---------|--------------|------------------|-----------|-------|-----------|-----------|---------|\n";

    sovereign::milp::MILPModel val_milp("PARALLEL_VAL_MILP", sovereign::ObjectiveSense::Minimize);
    sovereign::Index v1 = val_milp.add_variable("v1", 0.0, 2.0, 3.0, sovereign::VariableType::Integer);
    sovereign::Index v2 = val_milp.add_variable("v2", 0.0, 2.0, 4.0, sovereign::VariableType::Integer);
    sovereign::Index v3 = val_milp.add_variable("v3", 0.0, 2.0, 5.0, sovereign::VariableType::Integer);
    sovereign::Index v4 = val_milp.add_variable("v4", 0.0, 2.0, 6.0, sovereign::VariableType::Integer);

    std::vector<sovereign::Index> vcols1 = {v1, v2};
    std::vector<sovereign::Float64> vvals1 = {1.0, 1.0};
    val_milp.add_constraint("vc1", sovereign::lp::ConstraintSense::GreaterEqual, 1.5, vcols1, vvals1);

    std::vector<sovereign::Index> vcols2 = {v3, v4};
    std::vector<sovereign::Float64> vvals2 = {1.0, 1.0};
    val_milp.add_constraint("vc2", sovereign::lp::ConstraintSense::GreaterEqual, 1.5, vcols2, vvals2);

    struct ValRunConfig {
        std::size_t threads;
        sovereign::milp::ParallelMode mode;
        std::string label;
    };

    std::vector<ValRunConfig> val_runs = {
        {1, sovereign::milp::ParallelMode::Serial, "Serial"},
        {2, sovereign::milp::ParallelMode::Deterministic, "Deterministic"},
        {4, sovereign::milp::ParallelMode::Deterministic, "Deterministic"},
        {2, sovereign::milp::ParallelMode::MaxThroughput, "MaxThroughput"},
        {4, sovereign::milp::ParallelMode::MaxThroughput, "MaxThroughput"}
    };

    for (const auto& vcfg_item : val_runs) {
        sovereign::milp::MILPConfig vcfg;
        vcfg.num_threads = vcfg_item.threads;
        vcfg.parallel_mode = vcfg_item.mode;
        vcfg.enable_cuts = false; // Disable root cuts to isolate B&B tree exploration

        sovereign::milp::MILPSolver vsolver(vcfg);

        auto vstart = std::chrono::high_resolution_clock::now();
        auto vres = vsolver.solve(val_milp);
        auto vend = std::chrono::high_resolution_clock::now();
        double vms = std::chrono::duration<double, std::milli>(vend - vstart).count();

        std::cout << "| " << std::setw(7) << vcfg_item.threads
                  << " | " << std::left << std::setw(12) << vcfg_item.label
                  << " | " << std::left << std::setw(16) << "PARALLEL_VAL_MILP"
                  << " | " << std::right << std::setw(9) << std::fixed << std::setprecision(2) << vres.objective_value
                  << " | " << std::setw(5) << vres.nodes_explored
                  << " | " << std::setw(9) << vres.lp_solves
                  << " | " << std::setw(9) << std::fixed << std::setprecision(3) << vms
                  << " | " << std::setw(7) << std::fixed << std::setprecision(4) << vres.mip_gap << " |\n";
    }
    std::cout << "========================================================================================================================\n";
    std::cout << std::flush;

    return 0;
}
