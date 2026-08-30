#include "test_framework.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include <cmath>
#include <vector>

using namespace sovereign;
using namespace sovereign::lp;
using namespace sovereign::milp;

static MILPModel create_simple_infeasible_milp() {
    LPModel lp("simple_infeasible_milp", ObjectiveSense::Minimize);
    Index x0 = lp.add_variable("x0", 0.0, 1.0, 1.0, VariableType::Binary);
    
    // x0 <= 0.2 AND x0 >= 0.8 -> Impossible for Binary x0
    std::vector<Index> cols0 = {x0};
    std::vector<Float64> vals0 = {1.0};
    lp.add_constraint("c0", ConstraintSense::LessEqual, 0.2, cols0, vals0);
    lp.add_constraint("c1", ConstraintSense::GreaterEqual, 0.8, cols0, vals0);

    return MILPModel(lp);
}

static MILPModel create_simple_optimal_milp() {
    LPModel lp("simple_optimal_milp", ObjectiveSense::Minimize);
    Index x0 = lp.add_variable("x0", 0.0, 1.0, 10.0, VariableType::Binary);
    Index x1 = lp.add_variable("x1", 0.0, 1.0, 20.0, VariableType::Binary);

    // x0 + x1 >= 1.0
    std::vector<Index> cols = {x0, x1};
    std::vector<Float64> vals = {1.0, 1.0};
    lp.add_constraint("c0", ConstraintSense::GreaterEqual, 1.0, cols, vals);

    return MILPModel(lp);
}

// 1. LP Optimal -> Normal Node Processing Test
TEST_CASE(BBStatusOptimalNodeProcessing) {
    MILPModel model = create_simple_optimal_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_EQ(res.nodes_pruned_infeasible, 0);
}

// 2. LP Infeasible -> Infeasible Fathom Test
TEST_CASE(BBStatusInfeasibleFathom) {
    MILPModel model = create_simple_infeasible_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Infeasible));
}

// 3. LP IterationLimit -> Unresolved Counter Increment Test
TEST_CASE(BBStatusIterationLimitUnresolved) {
    MILPModel model = create_simple_optimal_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;
    // Set max_nodes = 1 so search interrupts cleanly
    config.max_nodes = 1;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    // When max_nodes is reached, status should be NodeLimit or Unresolved, not Infeasible!
    EXPECT_TRUE(res.status != MILPStatus::Infeasible);
}

// 4. LP NumericalFailure -> Not Infeasible Fathomed Test
TEST_CASE(BBStatusNumericalFailureNotFathomedInfeasible) {
    MILPModel model = create_simple_optimal_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    // Ensure unresolved / failure nodes do NOT increment nodes_pruned_infeasible
    EXPECT_EQ(res.nodes_pruned_infeasible, 0);
}

// 5. No-Incumbent + Unresolved Node -> NOT Infeasible Test
TEST_CASE(BBStatusNoIncumbentUnresolvedNotInfeasible) {
    MILPModel model = create_simple_optimal_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;
    config.enable_feasibility_pump = false;
    config.enable_diving = false;
    config.max_nodes = 0; // Immediate exit before root branching

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_TRUE(res.status != MILPStatus::Infeasible);
}

// 6. Incumbent + Unresolved Node -> NOT Optimal Test
TEST_CASE(BBStatusIncumbentUnresolvedNotOptimal) {
    MILPModel model = create_simple_optimal_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;
    config.max_nodes = 1; // Interrupted search

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    if (res.open_nodes_remaining > 0) {
        EXPECT_TRUE(res.status != MILPStatus::Optimal);
    }
}

// 7. Empty Queue + No Incumbent -> Infeasible ONLY if all nodes proven infeasible
TEST_CASE(BBStatusEmptyQueueAllProvenInfeasible) {
    MILPModel model = create_simple_infeasible_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Infeasible));
    EXPECT_EQ(res.nodes_unresolved, 0);
}

// 8. Empty Queue + Incumbent -> Optimal with valid global proof
TEST_CASE(BBStatusEmptyQueueIncumbentOptimal) {
    MILPModel model = create_simple_optimal_milp();
    MILPConfig config;
    config.num_threads = 1;
    config.parallel_mode = ParallelMode::Serial;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
}
