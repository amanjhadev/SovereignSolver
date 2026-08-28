#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/mps_parser.hpp"
#include "sovereign/lp/lp_validator.hpp"
#include <vector>

TEST_CASE(DualSimplexUnboundedLP) {
    sovereign::lp::LPModel model("unbounded_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, sovereign::INFINITY_VAL, -1.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, sovereign::INFINITY_VAL, -1.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> vals = {1.0, -1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 1.0, cols, vals);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Unbounded));
}

TEST_CASE(DualSimplexDegenerateLP) {
    sovereign::lp::LPModel model("degenerate_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, -2.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, -3.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};
    std::vector<double> v2 = {2.0, 1.0};
    std::vector<double> v3 = {1.0, 2.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 2.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 4.0, cols, v2);
    model.add_constraint("c3", sovereign::lp::ConstraintSense::LessEqual, 4.0, cols, v3);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, -6.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], 0.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-5);
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_TRUE(res.is_dual_feasible);
    EXPECT_NEAR(res.max_primal_residual, 0.0, 1e-5);
    EXPECT_NEAR(res.max_dual_residual, 0.0, 1e-5);
}

TEST_CASE(DualSimplexMultipleOptimaLP) {
    sovereign::lp::LPModel model("multiple_optima_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 4.0, -1.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 4.0, -1.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 5.0, cols, v1);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, -5.0, 1e-5);
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_TRUE(res.is_dual_feasible);
    EXPECT_NEAR(res.max_primal_residual, 0.0, 1e-5);
    EXPECT_NEAR(res.max_dual_residual, 0.0, 1e-5);
}

TEST_CASE(DualSimplexBFRTBoundFlip) {
    sovereign::lp::LPModel model("bfrt_bound_flip_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 2.0, 0.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 5.0);
    sovereign::Index x3 = model.add_variable("x3", 0.0, 10.0, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2, x3};
    std::vector<double> v1 = {1.0, 1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::Equal, 15.0, cols, v1);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 65.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2] + res.primal_solution[x3], 13.0, 1e-5);
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.max_primal_residual, 0.0, 1e-5);
}

TEST_CASE(BenchmarkHarnessMPSSolve) {
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_file("tests/fixtures/sample_benchmark.mps", model);
    EXPECT_STATUS_OK(status);

    auto val_res = sovereign::lp::LPValidator::validate(model);
    EXPECT_TRUE(val_res.is_valid);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.max_primal_residual, 0.0, 1e-5);
}
