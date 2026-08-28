#include "test_framework.hpp"
#include "sovereign/core/thread_pool.hpp"
#include "sovereign/milp/atomic_incumbent.hpp"
#include "sovereign/milp/concurrent_cut_pool.hpp"
#include "sovereign/milp/work_stealing_queue.hpp"
#include "sovereign/milp/parallel_milp_solver.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <cmath>

using namespace sovereign;
using namespace sovereign::milp;

TEST_CASE(ParallelThreadPoolBasic) {
    core::ThreadPool pool(4);
    EXPECT_EQ(pool.size(), 4);

    std::atomic<int> counter{0};
    for (int i = 0; i < 20; ++i) {
        pool.enqueue([&counter](std::stop_token st) {
            (void)st;
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.wait_all();
    EXPECT_EQ(counter.load(), 20);
}

TEST_CASE(AtomicIncumbentThreadSafety) {
    AtomicIncumbent incumbent;
    EXPECT_FALSE(incumbent.has_incumbent());

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&incumbent, i]() {
            std::vector<Float64> sol = {static_cast<Float64>(i), static_cast<Float64>(i * 2)};
            Float64 obj = 100.0 - static_cast<Float64>(i * 10); // Decreasing objective values
            incumbent.update_incumbent(sol, obj, ObjectiveSense::Minimize, 1e-6);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_TRUE(incumbent.has_incumbent());
    // Minimum obj submitted was 30.0 (from i=7)
    EXPECT_NEAR(incumbent.best_objective(), 30.0, 1e-6);
    auto sol = incumbent.get_solution();
    EXPECT_EQ(sol.size(), 2);
    EXPECT_NEAR(sol[0], 7.0, 1e-6);
    EXPECT_NEAR(sol[1], 14.0, 1e-6);
}

TEST_CASE(ConcurrentCutPoolReaderWriter) {
    ConcurrentCutPool cut_pool;
    std::vector<Float64> lp_sol = {1.5, 2.5, 1.5, 2.5, 1.5, 2.5, 1.5, 2.5};

    std::vector<std::thread> writers;
    for (int i = 0; i < 4; ++i) {
        writers.emplace_back([&cut_pool, &lp_sol, i]() {
            Cut c;
            c.name = "cut_" + std::to_string(i);
            c.type = CutType::GomoryFractional;
            c.scope = CutScope::Global;
            c.origin_node_id = i;
            c.rhs = 1.0 + i;
            c.col_indices = {2 * i, 2 * i + 1};
            c.coefficients = {1.0, static_cast<Float64>(i + 1)};
            cut_pool.add_cut(c, lp_sol);
        });
    }

    for (auto& w : writers) {
        w.join();
    }

    EXPECT_TRUE(cut_pool.size() > 0);
    auto active_cuts = cut_pool.get_active_cuts();
    EXPECT_EQ(active_cuts.size(), cut_pool.size());
}

TEST_CASE(ThreadSafeNodeQueuePushPop) {
    ThreadSafeNodeQueue queue(NodeSelectionRule::BestBoundFirst, ObjectiveSense::Minimize);
    EXPECT_TRUE(queue.empty());

    auto n1 = std::make_shared<BBNode>(1, -1, 0, 10.0);
    auto n2 = std::make_shared<BBNode>(2, -1, 0, 5.0);
    auto n3 = std::make_shared<BBNode>(3, -1, 0, 15.0);

    queue.push(n1);
    queue.push(n2);
    queue.push(n3);

    EXPECT_EQ(queue.size(), 3);
    // Best bound for minimization should be n2 (bound = 5.0)
    auto top1 = queue.pop();
    EXPECT_EQ(top1->node_id, 2);
    EXPECT_NEAR(top1->lp_objective, 5.0, 1e-6);

    auto top2 = queue.pop();
    EXPECT_EQ(top2->node_id, 1);
    EXPECT_NEAR(top2->lp_objective, 10.0, 1e-6);

    auto top3 = queue.pop();
    EXPECT_EQ(top3->node_id, 3);
    EXPECT_NEAR(top3->lp_objective, 15.0, 1e-6);

    EXPECT_TRUE(queue.empty());
}

TEST_CASE(ParallelMILPDeterministicMode) {
    // Binary Knapsack Model: Maximize 10*x1 + 15*x2 + 25*x3 + 20*x4
    // s.t. 3*x1 + 4*x2 + 6*x3 + 5*x4 <= 10
    MILPModel model("knapsack_det", ObjectiveSense::Maximize);
    Index x1 = model.add_variable("x1", 0.0, 1.0, 10.0, VariableType::Binary);
    Index x2 = model.add_variable("x2", 0.0, 1.0, 15.0, VariableType::Binary);
    Index x3 = model.add_variable("x3", 0.0, 1.0, 25.0, VariableType::Binary);
    Index x4 = model.add_variable("x4", 0.0, 1.0, 20.0, VariableType::Binary);

    std::vector<Index> cols = {x1, x2, x3, x4};
    std::vector<Float64> vals = {3.0, 4.0, 6.0, 5.0};
    model.add_constraint("c1", lp::ConstraintSense::LessEqual, 10.0, cols, vals);

    MILPConfig config;
    config.num_threads = 4;
    config.parallel_mode = ParallelMode::Deterministic;
    config.random_seed = 42;

    ParallelMILPSolver solver(config);
    MILPResult res1 = solver.solve(model);
    MILPResult res2 = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res1.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_EQ(static_cast<int>(res2.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res1.objective_value, 40.0, 1e-4);
    EXPECT_NEAR(res2.objective_value, 40.0, 1e-4);

    // Reproducibility check between runs in Deterministic mode
    EXPECT_EQ(res1.nodes_explored, res2.nodes_explored);
    EXPECT_EQ(res1.lp_solves, res2.lp_solves);
}

TEST_CASE(ParallelVsSerialEquivalence) {
    // Minimization MILP: min -x1 - x2 s.t. 2*x1 + x2 <= 9, x1 + 2*x2 <= 7, x1, x2 >= 0 integer
    MILPModel model("milp_equiv", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -1.0, VariableType::Integer);

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {2.0, 1.0};
    model.add_constraint("c1", lp::ConstraintSense::LessEqual, 9.0, cols1, vals1);

    std::vector<Index> cols2 = {x1, x2};
    std::vector<Float64> vals2 = {1.0, 2.0};
    model.add_constraint("c2", lp::ConstraintSense::LessEqual, 7.0, cols2, vals2);

    MILPConfig serial_config;
    serial_config.num_threads = 1;
    serial_config.parallel_mode = ParallelMode::Serial;

    MILPSolver serial_solver(serial_config);
    MILPResult serial_res = serial_solver.solve(model);

    MILPConfig parallel_config;
    parallel_config.num_threads = 4;
    parallel_config.parallel_mode = ParallelMode::Deterministic;

    ParallelMILPSolver parallel_solver(parallel_config);
    MILPResult parallel_res = parallel_solver.solve(model);

    EXPECT_EQ(static_cast<int>(serial_res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_EQ(static_cast<int>(parallel_res.status), static_cast<int>(MILPStatus::Optimal));

    EXPECT_NEAR(serial_res.objective_value, -5.0, 1e-4);
    EXPECT_NEAR(parallel_res.objective_value, -5.0, 1e-4);
    EXPECT_NEAR(serial_res.objective_value, parallel_res.objective_value, 1e-6);
}

TEST_CASE(ParallelValMILPMultiThreadValidation) {
    // Dedicated Multi-Node Validation MILP
    MILPModel val_milp("PARALLEL_VAL_MILP", ObjectiveSense::Minimize);
    Index v1 = val_milp.add_variable("v1", 0.0, 2.0, 3.0, VariableType::Integer);
    Index v2 = val_milp.add_variable("v2", 0.0, 2.0, 4.0, VariableType::Integer);
    Index v3 = val_milp.add_variable("v3", 0.0, 2.0, 5.0, VariableType::Integer);
    Index v4 = val_milp.add_variable("v4", 0.0, 2.0, 6.0, VariableType::Integer);

    std::vector<Index> vcols1 = {v1, v2};
    std::vector<Float64> vvals1 = {1.0, 1.0};
    val_milp.add_constraint("vc1", lp::ConstraintSense::GreaterEqual, 1.5, vcols1, vvals1);

    std::vector<Index> vcols2 = {v3, v4};
    std::vector<Float64> vvals2 = {1.0, 1.0};
    val_milp.add_constraint("vc2", lp::ConstraintSense::GreaterEqual, 1.5, vcols2, vvals2);

    // 1. Serial Solve
    MILPConfig cfg_serial;
    cfg_serial.num_threads = 1;
    cfg_serial.parallel_mode = ParallelMode::Serial;
    cfg_serial.enable_cuts = false;

    MILPSolver solver_serial(cfg_serial);
    MILPResult res_serial = solver_serial.solve(val_milp);

    EXPECT_EQ(static_cast<int>(res_serial.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res_serial.objective_value, 16.0, 1e-4);
    EXPECT_TRUE(res_serial.nodes_explored >= 4);

    // 2. 2-Thread Deterministic
    MILPConfig cfg_det2;
    cfg_det2.num_threads = 2;
    cfg_det2.parallel_mode = ParallelMode::Deterministic;
    cfg_det2.enable_cuts = false;

    ParallelMILPSolver solver_det2(cfg_det2);
    MILPResult res_det2 = solver_det2.solve(val_milp);

    EXPECT_EQ(static_cast<int>(res_det2.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res_det2.objective_value, 16.0, 1e-4);
    EXPECT_TRUE(res_det2.nodes_explored >= 4);

    // 3. 4-Thread Deterministic
    MILPConfig cfg_det4;
    cfg_det4.num_threads = 4;
    cfg_det4.parallel_mode = ParallelMode::Deterministic;
    cfg_det4.enable_cuts = false;

    ParallelMILPSolver solver_det4(cfg_det4);
    MILPResult res_det4 = solver_det4.solve(val_milp);

    EXPECT_EQ(static_cast<int>(res_det4.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res_det4.objective_value, 16.0, 1e-4);
    EXPECT_TRUE(res_det4.nodes_explored >= 4);

    // 4. 2-Thread MaxThroughput
    MILPConfig cfg_tp2;
    cfg_tp2.num_threads = 2;
    cfg_tp2.parallel_mode = ParallelMode::MaxThroughput;
    cfg_tp2.enable_cuts = false;

    ParallelMILPSolver solver_tp2(cfg_tp2);
    MILPResult res_tp2 = solver_tp2.solve(val_milp);

    EXPECT_EQ(static_cast<int>(res_tp2.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res_tp2.objective_value, 16.0, 1e-4);
    EXPECT_TRUE(res_tp2.nodes_explored >= 4);

    // Objective equivalence across all modes
    EXPECT_NEAR(res_serial.objective_value, res_det2.objective_value, 1e-6);
    EXPECT_NEAR(res_serial.objective_value, res_det4.objective_value, 1e-6);
    EXPECT_NEAR(res_serial.objective_value, res_tp2.objective_value, 1e-6);
}
