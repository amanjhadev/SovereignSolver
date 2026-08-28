#include "test_framework.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/linalg/csr_matrix.hpp"
#include "sovereign/linalg/sparse_matrix.hpp"
#include <vector>

TEST_CASE(SparseMatrixCOOtoCSCtoCSRConversions) {
    // Construct 3x3 matrix:
    // [ 1.0  0.0  2.0 ]
    // [ 0.0  3.0  0.0 ]
    // [ 4.0  0.0  5.0 ]
    sovereign::linalg::COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 2, 2.0);
    coo.add_entry(1, 1, 3.0);
    coo.add_entry(2, 0, 4.0);
    coo.add_entry(2, 2, 5.0);

    EXPECT_EQ(coo.num_rows(), 3);
    EXPECT_EQ(coo.num_cols(), 3);
    EXPECT_EQ(coo.num_nonzeros(), 5);

    auto csc = coo.to_csc();
    EXPECT_TRUE(csc.is_valid());
    EXPECT_EQ(csc.num_rows(), 3);
    EXPECT_EQ(csc.num_cols(), 3);
    EXPECT_EQ(csc.num_nonzeros(), 5);

    auto csr = coo.to_csr();
    EXPECT_TRUE(csr.is_valid());
    EXPECT_EQ(csr.num_rows(), 3);
    EXPECT_EQ(csr.num_cols(), 3);
    EXPECT_EQ(csr.num_nonzeros(), 5);

    // Convert CSR back to CSC and check validity
    auto csc2 = csr.to_csc();
    EXPECT_TRUE(csc2.is_valid());
    EXPECT_EQ(csc2.num_nonzeros(), 5);
}

TEST_CASE(SparseMatrixSpMV) {
    // Matrix A:
    // [ 2.0  -1.0 ]
    // [ 1.0   3.0 ]
    sovereign::linalg::COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 2.0);
    coo.add_entry(0, 1, -1.0);
    coo.add_entry(1, 0, 1.0);
    coo.add_entry(1, 1, 3.0);

    auto csc = coo.to_csc();
    auto csr = coo.to_csr();

    std::vector<double> x = {2.0, 4.0};
    std::vector<double> y_csc(2, 0.0);
    std::vector<double> y_csr(2, 0.0);

    csc.multiply(x, y_csc); // [2*2 - 1*4, 1*2 + 3*4] = [0.0, 14.0]
    csr.multiply(x, y_csr);

    EXPECT_NEAR(y_csc[0], 0.0, 1e-12);
    EXPECT_NEAR(y_csc[1], 14.0, 1e-12);
    EXPECT_NEAR(y_csr[0], 0.0, 1e-12);
    EXPECT_NEAR(y_csr[1], 14.0, 1e-12);

    // Transpose multiply: y = A^T * x
    std::vector<double> yt_csc(2, 0.0);
    std::vector<double> yt_csr(2, 0.0);

    csc.multiply_transpose(x, yt_csc); // [2*2 + 1*4, -1*2 + 3*4] = [8.0, 10.0]
    csr.multiply_transpose(x, yt_csr);

    EXPECT_NEAR(yt_csc[0], 8.0, 1e-12);
    EXPECT_NEAR(yt_csc[1], 10.0, 1e-12);
    EXPECT_NEAR(yt_csr[0], 8.0, 1e-12);
    EXPECT_NEAR(yt_csr[1], 10.0, 1e-12);
}

TEST_CASE(SparseMatrixDuplicateTripletDeduplication) {
    sovereign::linalg::COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 0, 2.0); // Duplicate -> should sum to 3.0
    coo.add_entry(1, 1, 5.0);

    auto csc = coo.to_csc();
    EXPECT_TRUE(csc.is_valid());
    EXPECT_EQ(csc.num_nonzeros(), 2);

    auto vals0 = csc.get_col_values(0);
    EXPECT_NEAR(vals0[0], 3.0, 1e-12);
}

TEST_CASE(SparseMatrixScalingAndNorms) {
    sovereign::linalg::COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 3.0);
    coo.add_entry(0, 1, -4.0);
    coo.add_entry(1, 1, 12.0);

    auto csc = coo.to_csc();
    EXPECT_NEAR(csc.infinity_norm(), 12.0, 1e-12); // max(|3|+|-4|, |12|) = 12
    EXPECT_NEAR(csc.one_norm(), 16.0, 1e-12);      // max(|3|, |-4|+|12|) = 16

    std::vector<double> row_scales = {2.0, 0.5};
    csc.scale_rows(row_scales);
    // Matrix becomes:
    // [ 6.0  -8.0 ]
    // [ 0.0   6.0 ]
    std::vector<double> x = {1.0, 1.0};
    std::vector<double> y(2, 0.0);
    csc.multiply(x, y);

    EXPECT_NEAR(y[0], -2.0, 1e-12);
    EXPECT_NEAR(y[1], 6.0, 1e-12);
}
