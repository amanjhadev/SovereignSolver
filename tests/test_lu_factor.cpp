#include "test_framework.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/lu_factor.hpp"
#include "sovereign/linalg/residual.hpp"
#include "sovereign/core/vector_ops.hpp"
#include <random>
#include <vector>
#include <chrono>
#include <iostream>

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

    for (int i = 0; i < N; ++i) {
        coo.add_entry(i, i, 15.0 + val_dist(gen));
    }

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

    std::vector<double> x_exact(N);
    for (int i = 0; i < N; ++i) {
        x_exact[i] = static_cast<double>(i + 1);
    }

    std::vector<double> b(N, 0.0);
    A.multiply(x_exact, b);

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

// ============================================================================
// Step 9: Reference vs New SparseLU Equivalence Verification Tests
// ============================================================================

TEST_CASE(ReferenceVsSparseEquivalenceDiagonal) {
    constexpr int N = 10;
    sovereign::linalg::COOMatrix coo(N, N);
    for (int i = 0; i < N; ++i) {
        coo.add_entry(i, i, static_cast<double>(i + 1) * 2.5);
    }
    auto A = coo.to_csc();

    sovereign::linalg::ReferenceDenseLU ref_lu;
    sovereign::linalg::SparseLU new_lu;

    EXPECT_STATUS_OK(ref_lu.factorize(A));
    EXPECT_STATUS_OK(new_lu.factorize(A));

    std::vector<double> b(N);
    for (int i = 0; i < N; ++i) b[i] = static_cast<double>(i + 3);

    std::vector<double> x_ref(N, 0.0), x_new(N, 0.0);
    EXPECT_STATUS_OK(ref_lu.solve(b, x_ref));
    EXPECT_STATUS_OK(new_lu.solve(b, x_new));

    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(x_new[i], x_ref[i], 1e-10);
    }
}

TEST_CASE(ReferenceVsSparseEquivalenceTriangular) {
    constexpr int N = 10;
    sovereign::linalg::COOMatrix coo(N, N);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j <= i; ++j) {
            coo.add_entry(i, j, static_cast<double>(i + j + 1));
        }
    }
    auto A = coo.to_csc();

    sovereign::linalg::ReferenceDenseLU ref_lu;
    sovereign::linalg::SparseLU new_lu;

    EXPECT_STATUS_OK(ref_lu.factorize(A));
    EXPECT_STATUS_OK(new_lu.factorize(A));

    std::vector<double> b(N, 1.0);
    std::vector<double> x_ref(N, 0.0), x_new(N, 0.0);
    EXPECT_STATUS_OK(ref_lu.solve(b, x_ref));
    EXPECT_STATUS_OK(new_lu.solve(b, x_new));

    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(x_new[i], x_ref[i], 1e-10);
    }
}

TEST_CASE(ReferenceVsSparseEquivalenceTridiagonal) {
    constexpr int N = 20;
    sovereign::linalg::COOMatrix coo(N, N);
    for (int i = 0; i < N; ++i) {
        coo.add_entry(i, i, 4.0);
        if (i > 0) coo.add_entry(i, i - 1, -1.0);
        if (i < N - 1) coo.add_entry(i, i + 1, -1.0);
    }
    auto A = coo.to_csc();

    sovereign::linalg::ReferenceDenseLU ref_lu;
    sovereign::linalg::SparseLU new_lu;

    EXPECT_STATUS_OK(ref_lu.factorize(A));
    EXPECT_STATUS_OK(new_lu.factorize(A));

    std::vector<double> b(N, 2.0);
    std::vector<double> x_ref(N, 0.0), x_new(N, 0.0);
    EXPECT_STATUS_OK(ref_lu.solve(b, x_ref));
    EXPECT_STATUS_OK(new_lu.solve(b, x_new));

    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(x_new[i], x_ref[i], 1e-10);
    }
}

TEST_CASE(ReferenceVsSparseEquivalenceArrowhead) {
    constexpr int N = 15;
    sovereign::linalg::COOMatrix coo(N, N);
    for (int i = 0; i < N; ++i) {
        coo.add_entry(i, i, static_cast<double>(i + 5));
        if (i > 0) {
            coo.add_entry(0, i, 1.0);
            coo.add_entry(i, 0, 1.0);
        }
    }
    auto A = coo.to_csc();

    sovereign::linalg::ReferenceDenseLU ref_lu;
    sovereign::linalg::SparseLU new_lu;

    EXPECT_STATUS_OK(ref_lu.factorize(A));
    EXPECT_STATUS_OK(new_lu.factorize(A));

    std::vector<double> b(N, 5.0);
    std::vector<double> x_ref(N, 0.0), x_new(N, 0.0);
    EXPECT_STATUS_OK(ref_lu.solve(b, x_ref));
    EXPECT_STATUS_OK(new_lu.solve(b, x_new));

    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(x_new[i], x_ref[i], 1e-10);
    }
}

TEST_CASE(ReferenceVsSparseEquivalencePermutations) {
    constexpr int N = 5;
    sovereign::linalg::COOMatrix coo(N, N);
    // Anti-diagonal permutation matrix
    for (int i = 0; i < N; ++i) {
        coo.add_entry(i, N - 1 - i, static_cast<double>(i + 1));
    }
    auto A = coo.to_csc();

    sovereign::linalg::ReferenceDenseLU ref_lu;
    sovereign::linalg::SparseLU new_lu;

    EXPECT_STATUS_OK(ref_lu.factorize(A));
    EXPECT_STATUS_OK(new_lu.factorize(A));

    std::vector<double> b = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> x_ref(N, 0.0), x_new(N, 0.0);
    EXPECT_STATUS_OK(ref_lu.solve(b, x_ref));
    EXPECT_STATUS_OK(new_lu.solve(b, x_new));

    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(x_new[i], x_ref[i], 1e-10);
    }
}

TEST_CASE(ReferenceVsSparseEquivalenceSparseRandom) {
    constexpr int N = 30;
    sovereign::linalg::COOMatrix coo(N, N);
    std::mt19937 gen(123);
    std::uniform_real_distribution<double> val_dist(-5.0, 5.0);

    for (int i = 0; i < N; ++i) coo.add_entry(i, i, 10.0 + val_dist(gen));
    for (int k = 0; k < N * 3; ++k) {
        int r = gen() % N;
        int c = gen() % N;
        if (r != c) coo.add_entry(r, c, val_dist(gen));
    }
    auto A = coo.to_csc();

    sovereign::linalg::ReferenceDenseLU ref_lu;
    sovereign::linalg::SparseLU new_lu;

    EXPECT_STATUS_OK(ref_lu.factorize(A));
    EXPECT_STATUS_OK(new_lu.factorize(A));

    std::vector<double> b(N);
    for (int i = 0; i < N; ++i) b[i] = val_dist(gen);

    std::vector<double> x_ref(N, 0.0), x_new(N, 0.0);
    EXPECT_STATUS_OK(ref_lu.solve(b, x_ref));
    EXPECT_STATUS_OK(new_lu.solve(b, x_new));

    auto res_new = sovereign::linalg::evaluate_residual(A, x_new, b);
    EXPECT_NEAR(res_new.absolute_residual_inf, 0.0, 1e-9);

    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(x_new[i], x_ref[i], 1e-9);
    }
}

TEST_CASE(ReferenceVsSparseEquivalenceSingular) {
    sovereign::linalg::COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 1, 2.0);
    coo.add_entry(1, 0, 2.0);
    coo.add_entry(1, 1, 4.0); // Dependent row
    coo.add_entry(2, 2, 5.0);
    auto A = coo.to_csc();

    sovereign::linalg::ReferenceDenseLU ref_lu;
    sovereign::linalg::SparseLU new_lu;

    auto st_ref = ref_lu.factorize(A);
    auto st_new = new_lu.factorize(A);

    EXPECT_EQ(st_ref, sovereign::Status::SingularMatrix);
    EXPECT_EQ(st_new, sovereign::Status::SingularMatrix);
}
