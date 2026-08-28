#include "test_framework.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/lu_factor.hpp"
#include "sovereign/linalg/residual.hpp"
#include "sovereign/core/vector_ops.hpp"
#include <random>
#include <vector>

TEST_CASE(LUSolveSmallHandcraftedSystems) {
    // 2x2 System:
    // [ 2.0   1.0 ] [x0] = [ 5.0 ] -> x = [2.0, 1.0]
    // [ 1.0  -1.0 ] [x1]   [ 1.0 ]
    sovereign::linalg::COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 2.0);
    coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 0, 1.0);
    coo.add_entry(1, 1, -1.0);

    auto A = coo.to_csc();
    sovereign::linalg::SparseLU lu;
    auto status = lu.factorize(A);

    EXPECT_STATUS_OK(status);
    EXPECT_TRUE(lu.is_factored());

    std::vector<double> b = {5.0, 1.0};
    std::vector<double> x(2, 0.0);

    status = lu.solve(b, x);
    EXPECT_STATUS_OK(status);

    EXPECT_NEAR(x[0], 2.0, 1e-10);
    EXPECT_NEAR(x[1], 1.0, 1e-10);

    auto residual = sovereign::linalg::evaluate_residual(A, x, b);
    EXPECT_NEAR(residual.absolute_residual_inf, 0.0, 1e-10);
}

TEST_CASE(LUSolve3x3SystemWithPermutation) {
    // 3x3 System requiring row/col swapping:
    // [ 0.0   2.0   1.0 ] [x0]   [ 4.0 ]
    // [ 1.0   0.0   0.0 ] [x1] = [ 1.0 ]
    // [ 3.0   0.0   4.0 ] [x2]   [ 11.0]
    // Solution: x0 = 1.0, x1 = 1.0, x2 = 2.0
    sovereign::linalg::COOMatrix coo(3, 3);
    coo.add_entry(0, 1, 2.0);
    coo.add_entry(0, 2, 1.0);
    coo.add_entry(1, 0, 1.0);
    coo.add_entry(2, 0, 3.0);
    coo.add_entry(2, 2, 4.0);

    auto A = coo.to_csc();
    sovereign::linalg::SparseLU lu;
    auto status = lu.factorize(A);

    EXPECT_STATUS_OK(status);

    std::vector<double> b = {4.0, 1.0, 11.0};
    std::vector<double> x(3, 0.0);

    status = lu.solve(b, x);
    EXPECT_STATUS_OK(status);

    EXPECT_NEAR(x[0], 1.0, 1e-10);
    EXPECT_NEAR(x[1], 1.0, 1e-10);
    EXPECT_NEAR(x[2], 2.0, 1e-10);

    auto residual = sovereign::linalg::evaluate_residual(A, x, b);
    EXPECT_NEAR(residual.absolute_residual_inf, 0.0, 1e-10);

    // Test Transpose solve A^T * x_t = b
    std::vector<double> xt(3, 0.0);
    status = lu.solve_transpose(b, xt);
    EXPECT_STATUS_OK(status);

    // Verify A^T * xt = b
    std::vector<double> at_xt(3, 0.0);
    A.multiply_transpose(xt, at_xt);
    for (int i = 0; i < 3; ++i) {
        EXPECT_NEAR(at_xt[i], b[i], 1e-10);
    }
}

TEST_CASE(LUSingularMatrixDetection) {
    // 2x2 Singular Matrix (linearly dependent rows):
    // [ 1.0   2.0 ]
    // [ 2.0   4.0 ]
    sovereign::linalg::COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 1, 2.0);
    coo.add_entry(1, 0, 2.0);
    coo.add_entry(1, 1, 4.0);

    auto A = coo.to_csc();
    sovereign::linalg::SparseLU lu;
    auto status = lu.factorize(A);

    EXPECT_FALSE(sovereign::is_ok(status));
    EXPECT_EQ(status, sovereign::Status::SingularMatrix);
    EXPECT_FALSE(lu.is_factored());
}

TEST_CASE(LURandomSparseMatrixAndResidualCheck) {
    constexpr int N = 25;
    sovereign::linalg::COOMatrix coo(N, N);

    std::mt19937 gen(42);
    std::uniform_real_distribution<double> val_dist(-10.0, 10.0);
    std::uniform_int_distribution<int> idx_dist(0, N - 1);

    // Ensure diagonal entries for non-singularity
    for (int i = 0; i < N; ++i) {
        coo.add_entry(i, i, 15.0 + val_dist(gen));
    }

    // Add random off-diagonal non-zeros (~15% density)
    for (int k = 0; k < N * 4; ++k) {
        int r = idx_dist(gen);
        int c = idx_dist(gen);
        if (r != c) {
            coo.add_entry(r, c, val_dist(gen));
        }
    }

    auto A = coo.to_csc();
    sovereign::linalg::SparseLU lu;
    auto status = lu.factorize(A);

    EXPECT_STATUS_OK(status);

    // Generate known solution x_exact = [1, 2, ..., N]
    std::vector<double> x_exact(N);
    for (int i = 0; i < N; ++i) {
        x_exact[i] = static_cast<double>(i + 1);
    }

    // Compute b = A * x_exact
    std::vector<double> b(N, 0.0);
    A.multiply(x_exact, b);

    // Solve for x
    std::vector<double> x_computed(N, 0.0);
    status = lu.solve(b, x_computed);
    EXPECT_STATUS_OK(status);

    auto residual = sovereign::linalg::evaluate_residual(A, x_computed, b);
    EXPECT_NEAR(residual.absolute_residual_inf, 0.0, 1e-9);
    EXPECT_NEAR(residual.relative_residual_inf, 0.0, 1e-9);

    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(x_computed[i], x_exact[i], 1e-9);
    }
}

TEST_CASE(LURepeatedSolvesMultipleRHS) {
    sovereign::linalg::COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 4.0);
    coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 1, 3.0);
    coo.add_entry(1, 2, 2.0);
    coo.add_entry(2, 0, 1.0);
    coo.add_entry(2, 2, 5.0);

    auto A = coo.to_csc();
    sovereign::linalg::SparseLU lu;
    auto status = lu.factorize(A);
    EXPECT_STATUS_OK(status);

    std::vector<std::vector<double>> rhs_list = {
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
        {5.0, -2.0, 10.0}
    };

    for (const auto& b : rhs_list) {
        std::vector<double> x(3, 0.0);
        status = lu.solve(b, x);
        EXPECT_STATUS_OK(status);

        auto res = sovereign::linalg::evaluate_residual(A, x, b);
        EXPECT_NEAR(res.absolute_residual_inf, 0.0, 1e-10);
    }
}

TEST_CASE(LUIterativeRefinementAccuracyImprovement) {
    sovereign::linalg::COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 10.0);
    coo.add_entry(0, 1, -7.0);
    coo.add_entry(1, 0, -3.0);
    coo.add_entry(1, 1, 2.0999999);
    coo.add_entry(1, 2, 6.0);
    coo.add_entry(2, 1, 5.0);
    coo.add_entry(2, 2, -1.0);

    auto A = coo.to_csc();
    sovereign::linalg::SparseLU lu;
    auto status = lu.factorize(A);
    EXPECT_STATUS_OK(status);

    std::vector<double> b = {3.0, 5.0, 4.0};
    std::vector<double> x(3, 0.0);
    lu.solve(b, x);

    auto initial_res = sovereign::linalg::evaluate_residual(A, x, b);

    status = sovereign::linalg::iterative_refinement(A, lu, b, x, 5, 1e-13);
    EXPECT_STATUS_OK(status);

    auto final_res = sovereign::linalg::evaluate_residual(A, x, b);
    EXPECT_TRUE(final_res.absolute_residual_inf <= initial_res.absolute_residual_inf + 1e-15);
}
