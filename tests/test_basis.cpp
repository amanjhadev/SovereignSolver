#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/basis.hpp"
#include "sovereign/lp/basis_matrix.hpp"
#include "sovereign/core/vector_ops.hpp"
#include <vector>

TEST_CASE(BasisStateTrackingAndValidation) {
    // 2 rows, 4 columns
    sovereign::lp::Basis basis(2, 4);

    basis.set_basic(2, 0); // Basic slot 0 -> col 2
    basis.set_basic(3, 1); // Basic slot 1 -> col 3

    basis.set_nonbasic(0, sovereign::lp::BasisStatus::AtLower);
    basis.set_nonbasic(1, sovereign::lp::BasisStatus::AtLower);

    basis.rebuild_position_maps();

    EXPECT_TRUE(basis.is_valid());
    EXPECT_EQ(basis.get_basic_var(0), 2);
    EXPECT_EQ(basis.get_basic_var(1), 3);
    EXPECT_EQ(basis.get_basic_pos(2), 0);
    EXPECT_EQ(basis.get_basic_pos(3), 1);
    EXPECT_EQ(static_cast<int>(basis.get_status(2)), static_cast<int>(sovereign::lp::BasisStatus::Basic));

    // Perform pivot: col 0 enters, slot 1 (col 3) leaves
    basis.pivot(0, 1, sovereign::lp::BasisStatus::AtUpper);

    EXPECT_TRUE(basis.is_valid());
    EXPECT_EQ(basis.get_basic_var(0), 2);
    EXPECT_EQ(basis.get_basic_var(1), 0);
    EXPECT_EQ(static_cast<int>(basis.get_status(3)), static_cast<int>(sovereign::lp::BasisStatus::AtUpper));
}

TEST_CASE(BasisMatrixSolves) {
    // LP Matrix A: 2x4 (Standard form with 2 slacks)
    // [ 2.0  1.0  1.0  0.0 ] [x0, x1, x2, x3]^T = [5.0, 4.0]^T
    // [ 1.0  3.0  0.0  1.0 ]
    sovereign::lp::LPModel model("basis_test");
    model.add_variable("x0", 0.0, 10.0, 3.0);
    model.add_variable("x1", 0.0, 10.0, 2.0);
    model.add_variable("x2", 0.0, sovereign::INFINITY_VAL, 0.0);
    model.add_variable("x3", 0.0, sovereign::INFINITY_VAL, 0.0);

    std::vector<sovereign::Index> c0 = {0, 1, 2, 3};
    std::vector<double> v0 = {2.0, 1.0, 1.0, 0.0};
    std::vector<double> v1 = {1.0, 3.0, 0.0, 1.0};

    model.add_constraint("c0", sovereign::lp::ConstraintSense::Equal, 5.0, c0, v0);
    model.add_constraint("c1", sovereign::lp::ConstraintSense::Equal, 4.0, c0, v1);

    // Initial logical basis: slacks x2 and x3 (cols 2 and 3)
    sovereign::lp::Basis basis(2, 4);
    basis.set_basic(2, 0);
    basis.set_basic(3, 1);
    basis.set_nonbasic(0, sovereign::lp::BasisStatus::AtLower);
    basis.set_nonbasic(1, sovereign::lp::BasisStatus::AtLower);
    basis.rebuild_position_maps();

    sovereign::lp::BasisMatrix B_mat;
    auto status = B_mat.factorize(model.get_csc_matrix(), basis);

    EXPECT_STATUS_OK(status);
    EXPECT_TRUE(B_mat.is_factored());

    // Primal solve: B * x_B = b -> x_B = [5.0, 4.0]
    std::vector<double> b = {5.0, 4.0};
    std::vector<double> x_B(2, 0.0);
    status = B_mat.solve_primal(b, x_B);
    EXPECT_STATUS_OK(status);
    EXPECT_NEAR(x_B[0], 5.0, 1e-10);
    EXPECT_NEAR(x_B[1], 4.0, 1e-10);

    // Dual solve: B^T * y = c_B -> c_B = [0.0, 0.0] -> y = [0.0, 0.0]
    std::vector<double> c_B = {0.0, 0.0};
    std::vector<double> y(2, 0.0);
    status = B_mat.solve_dual(c_B, y);
    EXPECT_STATUS_OK(status);
    EXPECT_NEAR(y[0], 0.0, 1e-10);
    EXPECT_NEAR(y[1], 0.0, 1e-10);
}
