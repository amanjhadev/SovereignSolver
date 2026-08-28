#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/presolve_solver.hpp"
#include "sovereign/presolve/presolve_engine.hpp"
#include "sovereign/presolve/postsolve_engine.hpp"
#include <vector>

using namespace sovereign;
using namespace sovereign::lp;
using namespace sovereign::presolve;

TEST_CASE(PresolveFixedVariableElimination) {
    LPModel model("fixed_var_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 2.0, 2.0, 5.0); // Fixed at 2.0
    Index x2 = model.add_variable("x2", 0.0, 10.0, 3.0);
    Index x3 = model.add_variable("x3", 0.0, 10.0, 4.0);

    // x1 + x2 + x3 <= 18  => 2.0 + x2 + x3 <= 18 => x2 + x3 <= 16.0
    std::vector<Index> cols = {x1, x2, x3};
    std::vector<Float64> vals = {1.0, 1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 18.0, cols, vals);

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);

    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Clean));
    EXPECT_EQ(pres_res.presolved_model.num_variables(), 2); // x1 eliminated, x2 and x3 remain
    EXPECT_NEAR(pres_res.presolved_model.objective_offset(), 10.0, 1e-6); // 5.0 * 2.0

    PresolveSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x3], 0.0, 1e-5);
    EXPECT_NEAR(res.objective_value, 10.0, 1e-5);
}

TEST_CASE(PresolveEmptyRowDetection) {
    LPModel model("empty_row_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    
    // Empty row 0 <= 5.0 (satisfied)
    std::vector<Index> empty_cols;
    std::vector<Float64> empty_vals;
    model.add_constraint("c_empty", ConstraintSense::LessEqual, 5.0, empty_cols, empty_vals);
    
    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 2.0, cols, vals);

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);

    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Clean));
    EXPECT_EQ(pres_res.presolved_model.num_constraints(), 1); // c_empty removed, c1 remains

    PresolveSolver solver;
    auto res = solver.solve(model);
    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 2.0, 1e-5);
}

TEST_CASE(PresolveEmptyColumnMinAndMax) {
    // 1. Minimization with positive objective coeff => fixed at lower bound 3.0
    LPModel model1("empty_col_min", ObjectiveSense::Minimize);
    Index x1 = model1.add_variable("x1", 3.0, 10.0, 4.0); // Empty column

    PresolveEngine presolver;
    auto pres_res1 = presolver.presolve(model1);

    EXPECT_EQ(static_cast<int>(pres_res1.status), static_cast<int>(PresolveStatus::Clean));
    EXPECT_EQ(pres_res1.presolved_model.num_variables(), 0);
    EXPECT_NEAR(pres_res1.presolved_model.objective_offset(), 12.0, 1e-6);

    PresolveSolver solver1;
    auto res1 = solver1.solve(model1);
    EXPECT_EQ(static_cast<int>(res1.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res1.primal_solution[x1], 3.0, 1e-5);
    EXPECT_NEAR(res1.objective_value, 12.0, 1e-5);

    // 2. Maximization with positive objective coeff => fixed at upper bound 10.0
    LPModel model2("empty_col_max", ObjectiveSense::Maximize);
    Index x2 = model2.add_variable("x2", 3.0, 10.0, 4.0); // Empty column

    auto pres_res2 = presolver.presolve(model2);
    EXPECT_EQ(static_cast<int>(pres_res2.status), static_cast<int>(PresolveStatus::Clean));
    EXPECT_NEAR(pres_res2.presolved_model.objective_offset(), 40.0, 1e-6);
}

TEST_CASE(PresolveRowSingletonBoundTightening) {
    LPModel model("row_singleton_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0);

    // 2 * x1 >= 6  => x1 >= 3.0
    std::vector<Index> cols1 = {x1};
    std::vector<Float64> vals1 = {2.0};
    model.add_constraint("c_single", ConstraintSense::GreaterEqual, 6.0, cols1, vals1);

    std::vector<Index> cols2 = {x1, x2};
    std::vector<Float64> vals2 = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 5.0, cols2, vals2);

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);

    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Clean));

    PresolveSolver solver;
    auto res = solver.solve(model);
    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.primal_solution[x1], 3.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-5);
    EXPECT_NEAR(res.objective_value, 13.0, 1e-5);
}

TEST_CASE(PresolveImpliedBoundTightening) {
    LPModel model("implied_bound_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0);

    // x1 + x2 <= 4.0  => x1 <= 4.0 (since x2 >= 0)
    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 4.0, cols, vals);

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);
    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Clean));

    PresolveSolver solver;
    auto res = solver.solve(model);
    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 0.0, 1e-5);
}

TEST_CASE(PresolveRedundantRowElimination) {
    LPModel model("redundant_row_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 2.0, 1.0);
    Index x2 = model.add_variable("x2", 0.0, 3.0, 1.0);

    // x1 + x2 <= 10.0 (redundant since x1 <= 2, x2 <= 3 => max sum = 5 <= 10)
    std::vector<Index> cols_red = {x1, x2};
    std::vector<Float64> vals_red = {1.0, 1.0};
    model.add_constraint("c_redundant", ConstraintSense::LessEqual, 10.0, cols_red, vals_red);

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 1.0, cols1, vals1);

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);

    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Clean));
    EXPECT_EQ(pres_res.presolved_model.num_constraints(), 1); // c_redundant eliminated, c1 remains

    PresolveSolver solver;
    auto res = solver.solve(model);
    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 1.0, 1e-5);
}

TEST_CASE(PresolveFreeColumnSingletonSubstitution) {
    LPModel model("free_col_singleton_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", -INFINITY_VAL, INFINITY_VAL, 3.0); // Free column singleton
    Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0);

    // x1 + 2 * x2 = 8.0  => x1 = 8.0 - 2 * x2
    // obj: 3*(8 - 2*x2) + 2*x2 = 24 - 4*x2
    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 2.0};
    model.add_constraint("c1", ConstraintSense::Equal, 8.0, cols, vals);

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);

    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Clean));
    EXPECT_EQ(pres_res.presolved_model.num_constraints(), 0); // c1 eliminated

    PresolveSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x2], 10.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], -12.0, 1e-5); // 8 - 2*10 = -12
    EXPECT_NEAR(res.objective_value, 3.0*(-12.0) + 2.0*10.0, 1e-5); // -16.0
}

TEST_CASE(PresolveInfeasibleDetection) {
    LPModel model("infeasible_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);

    // 1 * x1 >= 15.0 => x1 >= 15.0, but x1 <= 10.0 => Infeasible!
    std::vector<Index> cols = {x1};
    std::vector<Float64> vals = {1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 15.0, cols, vals);

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);

    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Infeasible));

    PresolveSolver solver;
    auto res = solver.solve(model);
    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Infeasible));
    EXPECT_EQ(res.iterations, 0); // Proved infeasible in presolve without running simplex!
}

TEST_CASE(PresolveUnboundedDetection) {
    LPModel model("unbounded_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", -INFINITY_VAL, INFINITY_VAL, -5.0); // Free empty column with c < 0

    PresolveEngine presolver;
    auto pres_res = presolver.presolve(model);

    EXPECT_EQ(static_cast<int>(pres_res.status), static_cast<int>(PresolveStatus::Unbounded));

    PresolveSolver solver;
    auto res = solver.solve(model);
    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Unbounded));
    EXPECT_EQ(res.iterations, 0); // Proved unbounded in presolve without running simplex!
}

TEST_CASE(PresolveEndToEndSolverIntegration) {
    LPModel model("end_to_end_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 2.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    Index x3 = model.add_variable("x3", 5.0, 5.0, 10.0); // Fixed variable

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {1.0, 2.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 6.0, cols1, vals1);

    std::vector<Index> cols2 = {x1, x2, x3};
    std::vector<Float64> vals2 = {3.0, 1.0, 0.0};
    model.add_constraint("c2", ConstraintSense::GreaterEqual, 8.0, cols2, vals2);

    std::vector<Index> cols_red = {x1, x2};
    std::vector<Float64> vals_red = {1.0, 1.0};
    model.add_constraint("c_redundant", ConstraintSense::LessEqual, 100.0, cols_red, vals_red);

    PresolveSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.max_primal_residual, 0.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x3], 5.0, 1e-5);
    EXPECT_NEAR(res.objective_value, 2.0*2.0 + 1.0*2.0 + 10.0*5.0, 1e-5); // 56.0
}
