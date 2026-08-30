#include "test_framework.hpp"
#include "sovereign/lp/basis_matrix.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/primal_simplex.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include <cmath>
#include <vector>
#include <random>

using namespace sovereign;
using namespace sovereign::lp;

TEST_CASE(BasisUpdateSinglePivotEta) {
    linalg::COOMatrix A_coo(3, 3);
    A_coo.add_entry(0, 0, 2.0);
    A_coo.add_entry(1, 1, 3.0);
    A_coo.add_entry(2, 2, 4.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(3, 3);
    basis.set_basic(0, 0);
    basis.set_basic(1, 1);
    basis.set_basic(2, 2);

    BasisMatrix B_mat;
    Status st = B_mat.factorize(A_csc, basis);
    EXPECT_STATUS_OK(st);

    std::vector<Float64> alpha = {1.0, 2.0, 0.5};
    Status up_st = B_mat.update_pivot(1, alpha, 1e-8);
    EXPECT_STATUS_OK(up_st);
    EXPECT_EQ(B_mat.update_count(), 1);

    std::vector<Float64> b = {4.0, 6.0, 8.0};
    std::vector<Float64> x_updated(3, 0.0);
    Status s1 = B_mat.solve_primal(b, x_updated);
    EXPECT_STATUS_OK(s1);

    linalg::COOMatrix B_prime_coo(3, 3);
    B_prime_coo.add_entry(0, 0, 2.0);
    B_prime_coo.add_entry(0, 1, 2.0);
    B_prime_coo.add_entry(1, 1, 6.0);
    B_prime_coo.add_entry(2, 1, 2.0);
    B_prime_coo.add_entry(2, 2, 4.0);
    auto B_prime_csc = B_prime_coo.to_csc();

    linalg::SparseLU lu_ref;
    lu_ref.factorize(B_prime_csc);
    std::vector<Float64> x_ref(3, 0.0);
    lu_ref.solve(b, x_ref);

    for (std::size_t i = 0; i < 3; ++i) {
        EXPECT_NEAR(x_updated[i], x_ref[i], 1e-10);
    }
}

TEST_CASE(BasisUpdate5ConsecutiveUpdates) {
    Index m = 5;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) {
        A_coo.add_entry(i, i, static_cast<Float64>(i + 1));
        if (i > 0) A_coo.add_entry(i - 1, i, 0.5);
    }
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> alpha1 = {0.5, 1.5, 2.0, 0.2, 0.0};
    B_mat.update_pivot(2, alpha1, 1e-8);

    std::vector<Float64> alpha2 = {1.0, 0.1, 2.0, 0.4, 0.0};
    B_mat.update_pivot(0, alpha2, 1e-8);

    std::vector<Float64> alpha3 = {1.0, 0.0, 0.0, 1.2, 0.8};
    B_mat.update_pivot(4, alpha3, 1e-8);

    std::vector<Float64> alpha4 = {0.2, 1.0, 0.0, 0.0, 0.5};
    B_mat.update_pivot(1, alpha4, 1e-8);

    std::vector<Float64> alpha5 = {0.0, 0.4, 0.1, 1.5, 0.2};
    B_mat.update_pivot(3, alpha5, 1e-8);

    EXPECT_EQ(B_mat.update_count(), 5);

    std::vector<Float64> b = {10.0, 15.0, 20.0, 25.0, 30.0};
    std::vector<Float64> x(m, 0.0);
    Status st_p = B_mat.solve_primal(b, x);
    EXPECT_STATUS_OK(st_p);

    std::vector<Float64> c_B = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<Float64> y(m, 0.0);
    Status st_d = B_mat.solve_dual(c_B, y);
    EXPECT_STATUS_OK(st_d);
}

TEST_CASE(BasisUpdate25ConsecutiveUpdates) {
    Index m = 10;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 2.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    SolverConfig cfg;
    cfg.max_refactorization_interval = 50;

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis, cfg);

    std::mt19937 rng(42);
    std::uniform_real_distribution<Float64> dist(0.5, 2.0);

    for (int k = 0; k < 25; ++k) {
        Index slot = k % m;
        std::vector<Float64> alpha(m, 0.0);
        for (Index i = 0; i < m; ++i) alpha[i] = dist(rng);
        alpha[slot] = 1.5; // Ensure non-zero pivot
        Status st = B_mat.update_pivot(slot, alpha, 1e-8);
        EXPECT_STATUS_OK(st);
    }

    EXPECT_EQ(B_mat.update_count(), 25);
}

TEST_CASE(BasisUpdate50UpdateBoundary) {
    Index m = 5;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    SolverConfig cfg;
    cfg.max_refactorization_interval = 50;

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis, cfg);
    EXPECT_FALSE(B_mat.should_refactorize());

    std::vector<Float64> alpha = {1.0, 0.5, 0.2, 0.1, 0.3};

    for (int k = 0; k < 49; ++k) {
        B_mat.update_pivot(k % m, alpha, 1e-8);
        EXPECT_FALSE(B_mat.should_refactorize());
    }

    B_mat.update_pivot(49 % m, alpha, 1e-8);
    EXPECT_TRUE(B_mat.should_refactorize()); // 50 updates reached

    B_mat.factorize(A_csc, basis, cfg);
    EXPECT_EQ(B_mat.update_count(), 0);
    EXPECT_FALSE(B_mat.should_refactorize());
}

TEST_CASE(BasisUpdateUnstablePivotDetection) {
    Index m = 3;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> alpha_tiny = {1.0, 1e-12, 0.5};
    Status st = B_mat.update_pivot(1, alpha_tiny, 1e-8);
    EXPECT_EQ(static_cast<int>(st), static_cast<int>(Status::NumericalInstability));
}

TEST_CASE(BasisUpdateDegeneratePivot) {
    Index m = 3;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    // Pivot with exactly 1.0 at slot 0 and zeros elsewhere (degenerate identity pivot)
    std::vector<Float64> alpha_deg = {1.0, 0.0, 0.0};
    Status st = B_mat.update_pivot(0, alpha_deg, 1e-8);
    EXPECT_STATUS_OK(st);
    EXPECT_EQ(B_mat.update_count(), 1);
}

TEST_CASE(BasisUpdateRandomSparseBasis) {
    Index m = 8;
    std::mt19937 rng(12345);
    std::uniform_real_distribution<Float64> dist(1.0, 5.0);

    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) {
        A_coo.add_entry(i, i, dist(rng));
        if (i + 1 < m) A_coo.add_entry(i + 1, i, 0.1 * dist(rng));
    }
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> b(m);
    for (Index i = 0; i < m; ++i) b[i] = dist(rng);

    for (int k = 0; k < 10; ++k) {
        Index slot = k % m;
        std::vector<Float64> alpha(m, 0.0);
        for (Index i = 0; i < m; ++i) alpha[i] = dist(rng);
        alpha[slot] = 3.0;
        B_mat.update_pivot(slot, alpha, 1e-8);
    }

    std::vector<Float64> x(m, 0.0);
    Status st = B_mat.solve_primal(b, x);
    EXPECT_STATUS_OK(st);
}

TEST_CASE(BasisUpdateIllConditionedBasis) {
    Index m = 3;
    linalg::COOMatrix A_coo(m, m);
    A_coo.add_entry(0, 0, 1e-4);
    A_coo.add_entry(1, 1, 1e4);
    A_coo.add_entry(2, 2, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> alpha = {1.0, 1.0, 1.0};
    Status st = B_mat.update_pivot(2, alpha, 1e-8);
    EXPECT_STATUS_OK(st);

    std::vector<Float64> b = {1e-4, 1e4, 1.0};
    std::vector<Float64> x(3, 0.0);
    Status st_x = B_mat.solve_primal(b, x);
    EXPECT_STATUS_OK(st_x);
}

TEST_CASE(BasisUpdateFTRANEquivalenceVsFreshLU) {
    Index m = 4;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 2.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> alpha = {0.5, 2.0, 0.0, 1.0};
    B_mat.update_pivot(1, alpha, 1e-8);

    std::vector<Float64> b = {2.0, 4.0, 6.0, 8.0};
    std::vector<Float64> x_eta(m, 0.0);
    B_mat.solve_primal(b, x_eta);

    // Build explicit B_prime
    linalg::COOMatrix B_prime(m, m);
    B_prime.add_entry(0, 0, 2.0);
    B_prime.add_entry(0, 1, 1.0); // 2.0 * 0.5
    B_prime.add_entry(1, 1, 4.0); // 2.0 * 2.0
    B_prime.add_entry(2, 2, 2.0);
    B_prime.add_entry(3, 1, 2.0); // 2.0 * 1.0
    B_prime.add_entry(3, 3, 2.0);
    auto B_prime_csc = B_prime.to_csc();

    linalg::SparseLU lu_ref;
    lu_ref.factorize(B_prime_csc);
    std::vector<Float64> x_ref(m, 0.0);
    lu_ref.solve(b, x_ref);

    for (Index i = 0; i < m; ++i) {
        EXPECT_NEAR(x_eta[i], x_ref[i], 1e-10);
    }
}

TEST_CASE(BasisUpdateBTRANEquivalenceVsFreshLU) {
    Index m = 4;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 2.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> alpha = {0.5, 2.0, 0.0, 1.0};
    B_mat.update_pivot(1, alpha, 1e-8);

    std::vector<Float64> c_B = {1.0, 3.0, 5.0, 7.0};
    std::vector<Float64> y_eta(m, 0.0);
    B_mat.solve_dual(c_B, y_eta);

    // Build explicit B_prime
    linalg::COOMatrix B_prime(m, m);
    B_prime.add_entry(0, 0, 2.0);
    B_prime.add_entry(0, 1, 1.0);
    B_prime.add_entry(1, 1, 4.0);
    B_prime.add_entry(2, 2, 2.0);
    B_prime.add_entry(3, 1, 2.0);
    B_prime.add_entry(3, 3, 2.0);
    auto B_prime_csc = B_prime.to_csc();

    linalg::SparseLU lu_ref;
    lu_ref.factorize(B_prime_csc);
    std::vector<Float64> y_ref(m, 0.0);
    lu_ref.solve_transpose(c_B, y_ref);

    for (Index i = 0; i < m; ++i) {
        EXPECT_NEAR(y_eta[i], y_ref[i], 1e-10);
    }
}

TEST_CASE(BasisUpdateRepeatedRHSSolves) {
    Index m = 3;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> alpha = {2.0, 1.0, 0.5};
    B_mat.update_pivot(0, alpha, 1e-8);

    std::vector<Float64> b1 = {1.0, 2.0, 3.0};
    std::vector<Float64> b2 = {10.0, 20.0, 30.0};

    std::vector<Float64> x1(3, 0.0), x2(3, 0.0);
    B_mat.solve_primal(b1, x1);
    B_mat.solve_primal(b2, x2);

    for (Index i = 0; i < 3; ++i) {
        EXPECT_NEAR(x2[i], 10.0 * x1[i], 1e-10);
    }
}

TEST_CASE(BasisUpdateBFRTCompatibility) {
    // Bound flips in BFRT execute bound flips without altering basic variables or calling update_pivot
    Index m = 3;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);
    EXPECT_EQ(B_mat.update_count(), 0);

    // Simulating BFRT bound flip: update_pivot is NOT called
    EXPECT_EQ(B_mat.update_count(), 0);
}

TEST_CASE(BasisUpdateDevexCompatibility) {
    Index m = 3;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    DevexPricing devex(m);
    std::vector<Float64> d = {1.0, 2.0, 0.5};
    devex.update_weights(1, d);

    B_mat.update_pivot(1, d, 1e-8);
    EXPECT_EQ(B_mat.update_count(), 1);
}

TEST_CASE(BasisUpdateRefactorizationResetsState) {
    Index m = 3;
    linalg::COOMatrix A_coo(m, m);
    for (Index i = 0; i < m; ++i) A_coo.add_entry(i, i, 1.0);
    auto A_csc = A_coo.to_csc();

    Basis basis(m, m);
    for (Index i = 0; i < m; ++i) basis.set_basic(i, i);

    BasisMatrix B_mat;
    B_mat.factorize(A_csc, basis);

    std::vector<Float64> alpha = {1.5, 0.5, 0.2};
    B_mat.update_pivot(0, alpha, 1e-8);
    B_mat.update_pivot(1, alpha, 1e-8);
    EXPECT_EQ(B_mat.update_count(), 2);

    B_mat.factorize(A_csc, basis);
    EXPECT_EQ(B_mat.update_count(), 0);
    EXPECT_TRUE(B_mat.is_factored());
}

TEST_CASE(BasisUpdateNoStaleBasisState) {
    BasisMatrix B_mat;
    EXPECT_FALSE(B_mat.is_factored());

    std::vector<Float64> b = {1.0, 2.0};
    std::vector<Float64> x(2);
    Status st = B_mat.solve_primal(b, x);
    EXPECT_EQ(static_cast<int>(st), static_cast<int>(Status::Error));
}
