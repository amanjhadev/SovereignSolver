#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/initial_basis.hpp"
#include "sovereign/lp/basis_matrix.hpp"
#include "sovereign/lp/mps_parser.hpp"
#include "sovereign/lp/lp_validator.hpp"
#include <vector>
#include <sstream>

TEST_CASE(InitialBasisEqualityConstraints) {
    // Model with 2 variables, 3 constraints: LE, EQ, GE
    // Row 0 (LE): x1 + 2 x2 <= 14  -> slack s0
    // Row 1 (EQ): 3 x1 - x2 == 5   -> no slack, needs structural col x1 or x2
    // Row 2 (GE): x1 + x2 >= 2     -> surplus s2
    sovereign::lp::LPModel model("mixed_constraints_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v0 = {1.0, 2.0};
    std::vector<double> v1 = {3.0, -1.0};
    std::vector<double> v2 = {1.0, 1.0};

    model.add_constraint("c0", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, v0);
    model.add_constraint("c1", sovereign::lp::ConstraintSense::Equal, 5.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::GreaterEqual, 2.0, cols, v2);

    auto std_model = model.convert_to_standard_form();
    auto basis = sovereign::lp::InitialBasisBuilder::construct_logical_basis(std_model);

    EXPECT_TRUE(basis.is_valid());
    EXPECT_EQ(basis.num_rows(), 3);

    sovereign::lp::BasisMatrix B_mat;
    auto status = sovereign::lp::InitialBasisBuilder::validate_initial_basis(std_model, basis, B_mat);

    EXPECT_STATUS_OK(status);
    EXPECT_TRUE(B_mat.is_factored());
}

TEST_CASE(InitialBasisMultipleEqualities) {
    // Model with 2 variables, 2 EQ constraints:
    // x1 + x2 == 5
    // 2 x1 - x2 == 1
    sovereign::lp::LPModel model("pure_eq_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v0 = {1.0, 1.0};
    std::vector<double> v1 = {2.0, -1.0};

    model.add_constraint("eq0", sovereign::lp::ConstraintSense::Equal, 5.0, cols, v0);
    model.add_constraint("eq1", sovereign::lp::ConstraintSense::Equal, 1.0, cols, v1);

    auto std_model = model.convert_to_standard_form();
    auto basis = sovereign::lp::InitialBasisBuilder::construct_logical_basis(std_model);

    EXPECT_TRUE(basis.is_valid());

    sovereign::lp::BasisMatrix B_mat;
    auto status = sovereign::lp::InitialBasisBuilder::validate_initial_basis(std_model, basis, B_mat);

    EXPECT_STATUS_OK(status);
    EXPECT_TRUE(B_mat.is_factored());
}

TEST_CASE(InitialBasisSingularCandidateDetection) {
    // Model with 2 variables and 2 linearly dependent EQ constraints:
    // x1 + x2 == 5
    // 2 x1 + 2 x2 == 10
    sovereign::lp::LPModel model("singular_eq_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v0 = {1.0, 1.0};
    std::vector<double> v1 = {2.0, 2.0};

    model.add_constraint("eq0", sovereign::lp::ConstraintSense::Equal, 5.0, cols, v0);
    model.add_constraint("eq1", sovereign::lp::ConstraintSense::Equal, 10.0, cols, v1);

    auto std_model = model.convert_to_standard_form();
    auto basis = sovereign::lp::InitialBasisBuilder::construct_logical_basis(std_model);

    sovereign::lp::BasisMatrix B_mat;
    auto status = sovereign::lp::InitialBasisBuilder::validate_initial_basis(std_model, basis, B_mat);

    // B matrix columns [1, 2]^T and [1, 2]^T are linearly dependent -> SingularMatrix
    EXPECT_EQ(static_cast<int>(status), static_cast<int>(sovereign::Status::SingularMatrix));
    EXPECT_FALSE(B_mat.is_factored());
}

TEST_CASE(FreeVariableTransformationAndSolutionRecovery) {
    // Model with 1 free variable x0 in (-inf, inf) and 1 non-negative variable x1 >= 0
    // Min 3.0 x0 + 5.0 x1
    // s.t. x0 + 2 x1 <= 10
    sovereign::lp::LPModel model("free_var_lp");
    sovereign::Index x0 = model.add_variable("x0", -sovereign::INFINITY_VAL, sovereign::INFINITY_VAL, 3.0);
    sovereign::Index x1 = model.add_variable("x1", 0.0, sovereign::INFINITY_VAL, 5.0);

    std::vector<sovereign::Index> cols = {x0, x1};
    std::vector<double> vals = {1.0, 2.0};
    model.add_constraint("c0", sovereign::lp::ConstraintSense::LessEqual, 10.0, cols, vals);

    auto std_model = model.convert_to_standard_form();

    // Standard model should have 3 structural/split variables (x0_pos, x0_neg, x1) + 1 slack (slack_c0) = 4 variables
    EXPECT_EQ(std_model.num_variables(), 4);
    EXPECT_EQ(std_model.num_constraints(), 1);

    // Verify objective coefficients for x0_pos (+3.0) and x0_neg (-3.0)
    EXPECT_EQ(std_model.get_variable(0).obj_coeff, 3.0);
    EXPECT_EQ(std_model.get_variable(1).obj_coeff, -3.0);

    // Test solution recovery map
    // Suppose standard form solution is x_std = [7.5, 2.5, 1.0, 0.0]
    // Recovered original solution: x0 = 7.5 - 2.5 = 5.0, x1 = 1.0
    std::vector<double> std_x = {7.5, 2.5, 1.0, 0.0};
    const auto& std_map = std_model.standard_form_map();
    auto orig_x = std_map.recover_original_solution(std_x);

    EXPECT_EQ(orig_x.size(), 2);
    EXPECT_NEAR(orig_x[0], 5.0, 1e-10);
    EXPECT_NEAR(orig_x[1], 1.0, 1e-10);
}

TEST_CASE(MPSRangesParsingValidAndInvalid) {
    // Valid MPS file with RANGES
    std::string valid_mps = R"(
NAME          RANGELP
ROWS
 N  COST
 L  ROW1
 G  ROW2
 E  ROW3
COLUMNS
    X1        COST      3.0   ROW1      1.0
    X1        ROW2      2.0   ROW3      1.0
RHS
    RHS1      ROW1     10.0   ROW2      5.0
    RHS1      ROW3      4.0
RANGES
    RNG1      ROW1      2.0   ROW2      3.0
    RNG1      ROW3      1.5
ENDATA
)";

    std::istringstream iss(valid_mps);
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, model, "valid_range_stream");
    EXPECT_STATUS_OK(status);

    // ROW1 (L, rhs 10.0, range 2.0) -> Range [8.0, 10.0]
    const auto& r1 = model.get_constraint(0);
    EXPECT_EQ(static_cast<int>(r1.sense), static_cast<int>(sovereign::lp::ConstraintSense::Range));
    EXPECT_NEAR(r1.lhs, 8.0, 1e-10);
    EXPECT_NEAR(r1.rhs, 10.0, 1e-10);

    // ROW2 (G, lhs 5.0, range 3.0) -> Range [5.0, 8.0]
    const auto& r2 = model.get_constraint(1);
    EXPECT_EQ(static_cast<int>(r2.sense), static_cast<int>(sovereign::lp::ConstraintSense::Range));
    EXPECT_NEAR(r2.lhs, 5.0, 1e-10);
    EXPECT_NEAR(r2.rhs, 8.0, 1e-10);

    // ROW3 (E, rhs 4.0, range 1.5) -> Range [4.0, 5.5]
    const auto& r3 = model.get_constraint(2);
    EXPECT_EQ(static_cast<int>(r3.sense), static_cast<int>(sovereign::lp::ConstraintSense::Range));
    EXPECT_NEAR(r3.lhs, 4.0, 1e-10);
    EXPECT_NEAR(r3.rhs, 5.5, 1e-10);

    // Invalid MPS file with non-existent row in RANGES
    std::string invalid_mps = R"(
NAME          BADRANGELP
ROWS
 N  COST
 L  ROW1
COLUMNS
    X1        COST      3.0   ROW1      1.0
RHS
    RHS1      ROW1     10.0
RANGES
    RNG1      NON_EXISTENT_ROW   2.0
ENDATA
)";

    std::istringstream iss_bad(invalid_mps);
    sovereign::lp::LPModel model_bad;
    status = parser.parse_stream(iss_bad, model_bad, "invalid_range_stream");
    EXPECT_FALSE(sovereign::is_ok(status));
    EXPECT_TRUE(!parser.last_error().empty());
}
