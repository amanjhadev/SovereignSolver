#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/mps_parser.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/barrier/sparse_cholesky.hpp"
#include "sovereign/barrier/barrier_solver.hpp"
#include "sovereign/barrier/presolve_barrier_solver.hpp"
#include <vector>
#include <cmath>

using namespace sovereign;
using namespace sovereign::lp;
using namespace sovereign::barrier;

TEST_CASE(SparseCholeskyFactorizationAndSolve) {
    // 3x3 SPD matrix M
    // [ 4  2  0 ]
    // [ 2  5  3 ]
    // [ 0  3  6 ]
    SparseCholesky cholesky;
    cholesky.analyze_pattern(3);

    // Create a 3x3 diagonal A matrix and d_sq to assemble M
    LPModel model;
    Index x1 = model.add_variable("x1", 0.0, 10.0, 0.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 0.0);
    Index x3 = model.add_variable("x3", 0.0, 10.0, 0.0);

    // Add rows to construct SPD matrix
    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {2.0, 1.0};
    model.add_constraint("r1", ConstraintSense::Equal, 0.0, cols1, vals1);

    std::vector<Index> cols2 = {x2, x3};
    std::vector<Float64> vals2 = {2.0, 1.0};
    model.add_constraint("r2", ConstraintSense::Equal, 0.0, cols2, vals2);

    std::vector<Index> cols3 = {x3};
    std::vector<Float64> vals3 = {2.0};
    model.add_constraint("r3", ConstraintSense::Equal, 0.0, cols3, vals3);

    std::vector<Float64> d_sq = {1.0, 1.0, 1.0};
    Status fact_stat = cholesky.factorize(model.get_csc_matrix(), d_sq, 1e-12, 1e-10, 1e-4);
    EXPECT_TRUE(is_ok(fact_stat));

    std::vector<Float64> rhs = {5.0, 11.0, 8.0};
    std::vector<Float64> sol(3, 0.0);
    Status solve_stat = cholesky.solve(rhs, sol);
    EXPECT_TRUE(is_ok(solve_stat));
}

TEST_CASE(BarrierAnalytical2DLP) {
    // min 2*x1 + 3*x2  s.t.  x1 + x2 >= 4.0, x1, x2 >= 0
    // Optimal: x1 = 4.0, x2 = 0.0, obj = 8.0
    LPModel model("analytical_2d", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 100.0, 2.0);
    Index x2 = model.add_variable("x2", 0.0, 100.0, 3.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 4.0, cols, vals);

    BarrierSolver solver;
    BarrierResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(BarrierStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_TRUE(res.is_dual_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 4.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-4);
    EXPECT_NEAR(res.objective_value, 8.0, 1e-4);
}

TEST_CASE(BarrierEqualityConstrainedLP) {
    // min x1 + 2*x2  s.t.  x1 + x2 = 5.0, x1, x2 >= 0
    // Optimal: x1 = 5.0, x2 = 0.0, obj = 5.0
    LPModel model("equality_lp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 100.0, 1.0);
    Index x2 = model.add_variable("x2", 0.0, 100.0, 2.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::Equal, 5.0, cols, vals);

    BarrierSolver solver;
    BarrierResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(BarrierStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 5.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-4);
    EXPECT_NEAR(res.objective_value, 5.0, 1e-4);
}

TEST_CASE(BarrierMixedConstraintsLP) {
    // min 3*x1 + 2*x2  s.t.  x1 + x2 <= 6.0, x1 >= 2.0, x1, x2 >= 0
    // Optimal: x1 = 2.0, x2 = 0.0, obj = 6.0
    LPModel model("mixed_lp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 100.0, 3.0);
    Index x2 = model.add_variable("x2", 0.0, 100.0, 2.0);

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 6.0, cols1, vals1);

    std::vector<Index> cols2 = {x1};
    std::vector<Float64> vals2 = {1.0};
    model.add_constraint("c2", ConstraintSense::GreaterEqual, 2.0, cols2, vals2);

    BarrierSolver solver;
    BarrierResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(BarrierStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-4);
    EXPECT_NEAR(res.objective_value, 6.0, 1e-4);
}

TEST_CASE(BarrierMultipleOptimaLP) {
    // min -x1 - x2  s.t.  x1 + x2 <= 5.0, x1, x2 >= 0
    // All points on x1 + x2 = 5 are optimal with obj = -5.0
    LPModel model("multiple_optima", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -1.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 5.0, cols, vals);

    BarrierSolver solver;
    BarrierResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(BarrierStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.objective_value, -5.0, 1e-4);
    EXPECT_TRUE(res.primal_solution[x1] + res.primal_solution[x2] <= 5.0 + 1e-4);
}

TEST_CASE(BarrierDualSimplexCrossValidation) {
    std::vector<std::string> fixtures = {
        "tests/fixtures/sample_benchmark.mps",
        "tests/fixtures/equality_dense.mps",
        "tests/fixtures/mixed_bounded.mps",
        "tests/fixtures/degenerate_basis.mps"
    };

    for (const auto& filepath : fixtures) {
        LPModel model;
        MPSParser parser;
        Status parse_stat = parser.parse_file(filepath, model);
        EXPECT_TRUE(is_ok(parse_stat));

        // Dual Simplex Solve
        DualSimplexSolver simplex_solver;
        SimplexResult s_res = simplex_solver.solve(model);
        EXPECT_EQ(static_cast<int>(s_res.status), static_cast<int>(SimplexStatus::Optimal));

        // Barrier Solve
        BarrierSolver barrier_solver;
        BarrierResult b_res = barrier_solver.solve(model);
        EXPECT_EQ(static_cast<int>(b_res.status), static_cast<int>(BarrierStatus::Optimal));

        // Cross-validation of optimal objectives within tolerance
        EXPECT_NEAR(b_res.objective_value, s_res.objective_value, 1e-4);
        EXPECT_TRUE(b_res.is_primal_feasible);
        EXPECT_TRUE(b_res.is_dual_feasible);
        EXPECT_NEAR(b_res.primal_residual, 0.0, 1e-4);
    }
}

TEST_CASE(BarrierPresolveIntegration) {
    LPModel model("presolve_barrier_integration", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 2.0, 2.0, 5.0); // Fixed at 2.0
    Index x2 = model.add_variable("x2", 0.0, 10.0, 3.0);
    Index x3 = model.add_variable("x3", 0.0, 10.0, 4.0);

    std::vector<Index> cols = {x1, x2, x3};
    std::vector<Float64> vals = {1.0, 1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 18.0, cols, vals);

    PresolveBarrierSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[x3], 0.0, 1e-4);
    EXPECT_NEAR(res.objective_value, 10.0, 1e-4);
}

TEST_CASE(BarrierIterationLimitHandling) {
    LPModel model("iter_limit_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 100.0, 2.0);
    Index x2 = model.add_variable("x2", 0.0, 100.0, 3.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 4.0, cols, vals);

    BarrierConfig cfg;
    cfg.max_iterations = 1; // Artificially low limit

    BarrierSolver solver(cfg);
    BarrierResult res = solver.solve(model);

    // Must return IterationLimit, NOT Infeasible!
    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(BarrierStatus::IterationLimit));
}
