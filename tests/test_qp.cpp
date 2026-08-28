#include "test_framework.hpp"
#include "sovereign/qp/qp_model.hpp"
#include "sovereign/qp/qp_solver.hpp"
#include "sovereign/qp/presolve_qp_solver.hpp"
#include <vector>
#include <cmath>

using namespace sovereign;
using namespace sovereign::lp;
using namespace sovereign::qp;

TEST_CASE(QPConvexityValidation) {
    // 1. Positive Definite Q (Convex)
    QPModel qp_pd("pd_qp", ObjectiveSense::Minimize);
    Index x1 = qp_pd.add_variable("x1", 0.0, 10.0, 1.0);
    Index x2 = qp_pd.add_variable("x2", 0.0, 10.0, 1.0);
    qp_pd.add_quadratic_term(x1, x1, 2.0);
    qp_pd.add_quadratic_term(x2, x2, 2.0);
    EXPECT_EQ(static_cast<int>(qp_pd.validate_convexity()), static_cast<int>(QPStatus::NotSolved));

    // 2. Singular Positive Semidefinite Q (Convex)
    QPModel qp_psd("psd_qp", ObjectiveSense::Minimize);
    Index y1 = qp_psd.add_variable("y1", 0.0, 10.0, 1.0);
    Index y2 = qp_psd.add_variable("y2", 0.0, 10.0, 1.0);
    // Q = [1  1; 1  1], rank 1, singular PSD!
    qp_psd.add_quadratic_term(y1, y1, 1.0);
    qp_psd.add_quadratic_term(y1, y2, 1.0);
    qp_psd.add_quadratic_term(y2, y2, 1.0);
    EXPECT_EQ(static_cast<int>(qp_psd.validate_convexity()), static_cast<int>(QPStatus::NotSolved));

    // 3. Indefinite Q (Non-convex -> MUST REJECT)
    QPModel qp_indef("indef_qp", ObjectiveSense::Minimize);
    Index z1 = qp_indef.add_variable("z1", 0.0, 10.0, 1.0);
    Index z2 = qp_indef.add_variable("z2", 0.0, 10.0, 1.0);
    // Q = [1  2; 2  1], det(Q) = 1 - 4 = -3 < 0 -> Indefinite!
    qp_indef.add_quadratic_term(z1, z1, 1.0);
    qp_indef.add_quadratic_term(z1, z2, 2.0);
    qp_indef.add_quadratic_term(z2, z2, 1.0);
    EXPECT_EQ(static_cast<int>(qp_indef.validate_convexity()), static_cast<int>(QPStatus::NonConvexModel));
}

TEST_CASE(QPSymmetricOperator) {
    QPModel qp("sym_op", ObjectiveSense::Minimize);
    Index x1 = qp.add_variable("x1", 0.0, 10.0, 2.0);
    Index x2 = qp.add_variable("x2", 0.0, 10.0, -3.0);

    // Q = [4  2; 2  6]
    qp.add_quadratic_term(x1, x1, 4.0);
    qp.add_quadratic_term(x1, x2, 2.0);
    qp.add_quadratic_term(x2, x2, 6.0);

    std::vector<Float64> vec_x = {1.0, 2.0};
    std::vector<Float64> Qx(2, 0.0);
    qp.Q_multiply(vec_x, Qx);

    // Q * x = [4*1 + 2*2, 2*1 + 6*2] = [8.0, 14.0]
    EXPECT_NEAR(Qx[0], 8.0, 1e-6);
    EXPECT_NEAR(Qx[1], 14.0, 1e-6);

    // Objective: 1/2 * (4*1^2 + 2*2*1*2 + 6*2^2) + (2*1 - 3*2) = 1/2 * (4 + 8 + 24) - 4 = 18 - 4 = 14.0
    Float64 obj = qp.evaluate_objective(vec_x);
    EXPECT_NEAR(obj, 14.0, 1e-6);

    // Gradient: Q*x + c = [8 + 2, 14 - 3] = [10.0, 11.0]
    std::vector<Float64> grad(2, 0.0);
    qp.evaluate_gradient(vec_x, grad);
    EXPECT_NEAR(grad[0], 10.0, 1e-6);
    EXPECT_NEAR(grad[1], 11.0, 1e-6);
}

TEST_CASE(QPAnalytical2D) {
    // min 1/2*(x1^2 + x2^2) - x1 - 2*x2  s.t.  x1 + x2 <= 2.0, x1, x2 >= 0
    // Analytical KKT solution: x1 = 0.5, x2 = 1.5, obj = -2.25
    QPModel model("analytical_qp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -2.0);

    model.add_quadratic_term(x1, x1, 1.0);
    model.add_quadratic_term(x2, x2, 1.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 2.0, cols, vals);

    QPSolver solver;
    QPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(QPStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 0.5, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 1.5, 1e-3);
    EXPECT_NEAR(res.objective_value, -2.25, 1e-3);
}

TEST_CASE(QPPortfolioOptimization) {
    // Portfolio optimization: min 1/2*(2*x1^2 + 2*x2^2) - (0.1*x1 + 0.2*x2)
    // s.t. x1 + x2 = 1.0, x1, x2 >= 0
    // Analytical KKT: x1 = 0.475, x2 = 0.525
    QPModel model("portfolio_qp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 1.0, -0.10);
    Index x2 = model.add_variable("x2", 0.0, 1.0, -0.20);

    model.add_quadratic_term(x1, x1, 2.0);
    model.add_quadratic_term(x2, x2, 2.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("budget", ConstraintSense::Equal, 1.0, cols, vals);

    QPSolver solver;
    QPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(QPStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 0.475, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 0.525, 1e-3);
}

TEST_CASE(QPEqualityConstrained) {
    // min 1/2*(4*x1^2 + 4*x2^2)  s.t.  x1 + 2*x2 = 5.0, x1, x2 >= 0
    // Analytical KKT: x1 = 1.0, x2 = 2.0, obj = 1/2*(4*1 + 4*4) = 10.0
    QPModel model("equality_qp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 0.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 0.0);

    model.add_quadratic_term(x1, x1, 4.0);
    model.add_quadratic_term(x2, x2, 4.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 2.0};
    model.add_constraint("c1", ConstraintSense::Equal, 5.0, cols, vals);

    QPSolver solver;
    QPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(QPStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 1.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 10.0, 1e-3);
}

TEST_CASE(QPBoundedVariables) {
    // min 1/2*(x1^2 + x2^2)  s.t.  x1 >= 2.0, x2 >= 3.0
    // Optimal: x1 = 2.0, x2 = 3.0, obj = 0.5*(4 + 9) = 6.5
    QPModel model("bounded_qp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 2.0, 10.0, 0.0);
    Index x2 = model.add_variable("x2", 3.0, 10.0, 0.0);

    model.add_quadratic_term(x1, x1, 1.0);
    model.add_quadratic_term(x2, x2, 1.0);

    QPSolver solver;
    QPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(QPStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 3.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 6.5, 1e-3);
}

TEST_CASE(QPPresolveIntegration) {
    // QP with fixed variable x1 = 2.0 and constraint x1 + x2 <= 18.0
    QPModel model("presolve_qp_integration", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 2.0, 2.0, 1.0); // Fixed at 2.0
    Index x2 = model.add_variable("x2", 0.0, 10.0, -2.0);

    model.add_quadratic_term(x1, x1, 2.0);
    model.add_quadratic_term(x2, x2, 1.0);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 18.0, cols, vals);

    PresolveQPSolver solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_TRUE(res.is_primal_feasible);
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-3);
}
