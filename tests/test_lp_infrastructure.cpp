#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/initial_basis.hpp"
#include "sovereign/lp/lp_validator.hpp"

TEST_CASE(InitialBasisConstructionAndValidation) {
    sovereign::lp::LPModel model("init_basis_test");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 2.0};
    std::vector<double> v2 = {3.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::LessEqual, 9.0, cols, v2);

    auto std_model = model.convert_to_standard_form();
    auto basis = sovereign::lp::InitialBasisBuilder::construct_logical_basis(std_model);

    EXPECT_TRUE(basis.is_valid());
    EXPECT_EQ(basis.num_rows(), 2);
    EXPECT_EQ(basis.num_cols(), 4);

    sovereign::lp::BasisMatrix B_mat;
    auto status = sovereign::lp::InitialBasisBuilder::validate_initial_basis(std_model, basis, B_mat);

    EXPECT_STATUS_OK(status);
    EXPECT_TRUE(B_mat.is_factored());
}

TEST_CASE(LPValidatorDetectionOfInvalidModels) {
    sovereign::lp::LPModel invalid_model("invalid_lp");
    
    // Add variable with lower bound > upper bound
    invalid_model.add_variable("bad_x", 10.0, 2.0, 1.0);

    auto val_res = sovereign::lp::LPValidator::validate(invalid_model);
    EXPECT_FALSE(val_res.is_valid);
    EXPECT_TRUE(!val_res.errors.empty());
}
