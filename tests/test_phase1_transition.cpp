#include "sovereign/public_solver_api.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/primal_simplex.hpp"
#include "sovereign/lp/mps_parser.hpp"
#include "test_framework.hpp"
#include <iostream>
#include <fstream>
#include <vector>

using namespace sovereign;
using namespace sovereign::lp;

TEST_CASE(Phase1TransitionValidStructuralPivot) {
    LPModel model("valid_pivot", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 2.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 3.0);
    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::Equal, 5.0, cols, vals);

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 10.0, 1e-4);
}

TEST_CASE(Phase1TransitionSingularBasisPrevention) {
    LPModel model("singular_prevention", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0);
    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals1 = {1.0, 2.0};
    std::vector<Float64> vals2 = {2.0, 4.0};
    model.add_constraint("c1", ConstraintSense::Equal, 4.0, cols, vals1);
    model.add_constraint("c2", ConstraintSense::Equal, 8.0, cols, vals2); // Dependent row

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
}

TEST_CASE(Phase1TransitionMultipleCandidatesStrongestPivot) {
    LPModel model("strongest_pivot", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 5.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    Index x3 = model.add_variable("x3", 0.0, 10.0, 10.0);
    std::vector<Index> cols = {x1, x2, x3};
    std::vector<Float64> vals = {0.001, 10.0, 0.5};
    model.add_constraint("c1", ConstraintSense::Equal, 6.0, cols, vals);

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 0.6, 1e-4); // Picks x2 = 0.6
}

TEST_CASE(Phase1TransitionDuplicateCandidatePrevention) {
    LPModel model("dup_prevention", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 5.0, 1.0);
    Index x2 = model.add_variable("x2", 0.0, 5.0, 1.0);
    std::vector<Index> c1 = {x1};
    std::vector<Index> c2 = {x2};
    std::vector<Float64> v = {1.0};
    model.add_constraint("c1", ConstraintSense::Equal, 2.0, c1, v);
    model.add_constraint("c2", ConstraintSense::Equal, 3.0, c2, v);

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 5.0, 1e-4);
}

TEST_CASE(Phase1TransitionRedundantRowsZeroArtificials) {
    LPModel model("redundant_zero_art", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 2.0);
    std::vector<Index> c = {x1};
    std::vector<Float64> v1 = {1.0};
    std::vector<Float64> v2 = {2.0};
    model.add_constraint("c1", ConstraintSense::Equal, 4.0, c, v1);
    model.add_constraint("c2", ConstraintSense::Equal, 8.0, c, v2);

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 8.0, 1e-4);
}

TEST_CASE(Phase1TransitionMultipleZeroArtificials) {
    LPModel model("multi_zero_art", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    std::vector<Index> c = {x1};
    std::vector<Float64> v1 = {1.0};
    std::vector<Float64> v2 = {2.0};
    std::vector<Float64> v3 = {3.0};
    model.add_constraint("c1", ConstraintSense::Equal, 3.0, c, v1);
    model.add_constraint("c2", ConstraintSense::Equal, 6.0, c, v2);
    model.add_constraint("c3", ConstraintSense::Equal, 9.0, c, v3);

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 9.0, 1e-4);
}

TEST_CASE(Phase1TransitionDegenerateEndingBasis) {
    LPModel model("degen_ending_basis", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0);
    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> v1 = {1.0, -1.0};
    std::vector<Float64> v2 = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::Equal, 0.0, cols, v1);
    model.add_constraint("c2", ConstraintSense::Equal, 5.0, cols, v2);

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 7.5, 1e-4);
}

TEST_CASE(Phase1TransitionFullFactorizationAndPFIReset) {
    LPModel model("factorize_pfi_reset", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 4.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {2.0, 1.0};
    model.add_constraint("c1", ConstraintSense::Equal, 4.0, cols, vals);

    DualSimplexSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 4.0, 1e-4);
}

TEST_CASE(Phase1Transition10TeamsCheckpointVerification) {
    MPSParser parser;
    LPModel model;
    Status parse_st = parser.parse_file("C:/Solver/10teams.mps", model);
    EXPECT_TRUE(is_ok(parse_st));

    SolverConfig config;
    config.max_iterations = 10000;
    DualSimplexSolver solver(config);
    SimplexResult res = solver.solve(model);

    // Verify Phase I successfully completes and Phase II begins/executes
    EXPECT_TRUE(res.status == SimplexStatus::Optimal || res.status == SimplexStatus::IterationLimit);
    EXPECT_TRUE(res.iterations > 0);
}
