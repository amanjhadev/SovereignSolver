#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/initial_basis.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include "sovereign/lp/ratio_test.hpp"
#include "sovereign/lp/devex_pricing.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include <vector>

TEST_CASE(SimplexStateInitializationAndSolves) {
    sovereign::lp::LPModel model("state_test_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 2.0};
    std::vector<double> v2 = {3.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 9.0, cols, v2);

    auto std_model = model.convert_to_standard_form();
    auto basis = sovereign::lp::InitialBasisBuilder::construct_logical_basis(std_model);

    sovereign::lp::SimplexState state(std_model, basis);
    EXPECT_EQ(state.num_rows(), 2);
    EXPECT_EQ(state.num_cols(), 4);

    auto status = state.factorize_basis();
    EXPECT_STATUS_OK(status);

    status = state.solve_primal_solution();
    EXPECT_STATUS_OK(status);

    status = state.solve_dual_solution();
    EXPECT_STATUS_OK(status);

    auto x_B = state.x_B();
    EXPECT_NEAR(x_B[0], 14.0, 1e-10);
    EXPECT_NEAR(x_B[1], 9.0, 1e-10);

    auto rc = state.reduced_costs();
    EXPECT_NEAR(rc[0], 3.0, 1e-10);
    EXPECT_NEAR(rc[1], 5.0, 1e-10);

    double max_viol = 0.0;
    sovereign::Index worst_idx = -1;
    bool is_p_feas = state.check_primal_feasibility(1e-6, max_viol, worst_idx);
    EXPECT_TRUE(is_p_feas);

    bool is_d_feas = state.check_dual_feasibility(1e-6, max_viol, worst_idx);
    EXPECT_TRUE(is_d_feas);
}

TEST_CASE(SimplexFTRANandBTRANCalculations) {
    sovereign::lp::LPModel model("ftran_btran_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 2.0};
    std::vector<double> v2 = {3.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 9.0, cols, v2);

    auto std_model = model.convert_to_standard_form();
    auto basis = sovereign::lp::InitialBasisBuilder::construct_logical_basis(std_model);

    sovereign::lp::SimplexState state(std_model, basis);
    EXPECT_STATUS_OK(state.factorize_basis());

    std::vector<double> d(2, 0.0);
    EXPECT_STATUS_OK(state.compute_ftran_direction(0, d));
    EXPECT_NEAR(d[0], 1.0, 1e-10);
    EXPECT_NEAR(d[1], 3.0, 1e-10);

    std::vector<double> y_0(2, 0.0);
    EXPECT_STATUS_OK(state.compute_btran_row(0, y_0));
    EXPECT_NEAR(y_0[0], 1.0, 1e-10);
    EXPECT_NEAR(y_0[1], 0.0, 1e-10);

    std::vector<double> alphas;
    state.compute_row_alphas(y_0, alphas);
    EXPECT_NEAR(alphas[0], 1.0, 1e-10);
    EXPECT_NEAR(alphas[1], 2.0, 1e-10);
}

TEST_CASE(SimplexDualHarrisRatioTest) {
    sovereign::lp::LPModel model("ratio_test_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 2.0};
    std::vector<double> v2 = {3.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 9.0, cols, v2);

    auto std_model = model.convert_to_standard_form();
    auto basis = sovereign::lp::InitialBasisBuilder::construct_logical_basis(std_model);

    sovereign::lp::SimplexState state(std_model, basis);
    EXPECT_STATUS_OK(state.factorize_basis());
    EXPECT_STATUS_OK(state.solve_primal_solution());
    EXPECT_STATUS_OK(state.solve_dual_solution());

    sovereign::lp::DualRatioTest ratio_tester;
    std::vector<double> y_0(2, 0.0);
    EXPECT_STATUS_OK(state.compute_btran_row(0, y_0));
    std::vector<double> alphas;
    state.compute_row_alphas(y_0, alphas);

    auto res = ratio_tester.select_entering_variable_harris(state, 0, alphas);
    EXPECT_FALSE(res.valid);
}

TEST_CASE(DualSimplexSimpleFeasibleLP) {
    sovereign::lp::LPModel model("simple_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 2.0};
    std::vector<double> v2 = {3.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 9.0, cols, v2);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 0.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x1], 0.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-6);
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_TRUE(res.is_dual_feasible);
}

TEST_CASE(DualSimplexMaxObjectiveLP) {
    sovereign::lp::LPModel model("max_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, -3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, -2.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};
    std::vector<double> v2 = {1.0, -1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 4.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 2.0, cols, v2);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, -11.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], 3.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 1.0, 1e-5);
    EXPECT_TRUE(res.is_primal_feasible);
}

TEST_CASE(DualSimplexEqualityConstrainedLP) {
    sovereign::lp::LPModel model("eq_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::Equal, 5.0, cols, v1);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 5.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], 5.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-5);
}

TEST_CASE(DualSimplexInfeasibleLP) {
    sovereign::lp::LPModel model("infeasible_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 2.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::GreaterEqual, 4.0, cols, v1);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Infeasible));
}

TEST_CASE(DualSimplexNonzeroLowerBoundLP) {
    sovereign::lp::LPModel model("nonzero_lb_lp");
    sovereign::Index x1 = model.add_variable("x1", 2.0, 10.0, 1.0);
    sovereign::Index x2 = model.add_variable("x2", 3.0, 10.0, 3.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 10.0, cols, v1);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 11.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 3.0, 1e-5);
}

TEST_CASE(DualSimplexBoxedVariableLP) {
    sovereign::lp::LPModel model("boxed_lp");
    sovereign::Index x1 = model.add_variable("x1", 1.0, 3.0, 2.0);
    sovereign::Index x2 = model.add_variable("x2", 2.0, 4.0, 3.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::GreaterEqual, 5.0, cols, v1);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 12.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], 3.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-5);
}

TEST_CASE(DualSimplexFreeVariableLP) {
    sovereign::lp::LPModel model("free_var_lp");
    sovereign::Index x1 = model.add_variable("x1", -sovereign::INFINITY_VAL, sovereign::INFINITY_VAL, 1.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 100.0, 2.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::Equal, 4.0, cols, v1);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 4.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x1], 4.0, 1e-5);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-5);
}

TEST_CASE(DualSimplexPhase1RequiredLP) {
    sovereign::lp::LPModel model("phase1_req_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 2.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 2.0};
    std::vector<double> v2 = {3.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::GreaterEqual, 6.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::GreaterEqual, 8.0, cols, v2);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_TRUE(res.is_dual_feasible);
}

TEST_CASE(DualSimplexRegressionStaleLUPostPivot) {
    sovereign::lp::LPModel model("multi_pivot_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, -2.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, -3.0);
    sovereign::Index x3 = model.add_variable("x3", 0.0, 10.0, -4.0);

    std::vector<sovereign::Index> cols = {x1, x2, x3};
    std::vector<double> v1 = {1.0, 2.0, 1.0};
    std::vector<double> v2 = {2.0, 1.0, 3.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 10.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 15.0, cols, v2);

    sovereign::lp::DualSimplexSolver solver;
    auto res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::lp::SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
}

TEST_CASE(DualSimplexRegressionDevexTinyPivot) {
    sovereign::lp::DevexPricing devex(2);
    std::vector<double> d_small = {1e-12, 2.0};
    devex.update_weights(0, d_small);
    sovereign::Float64 max_w = 0.0;
    (void)max_w;
}
