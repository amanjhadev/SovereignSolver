#include "test_framework.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/lp_validator.hpp"
#include <vector>

TEST_CASE(LPModelCreationAndInspection) {
    sovereign::lp::LPModel model("test_model", sovereign::ObjectiveSense::Minimize);
    
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, sovereign::INFINITY_VAL, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> vals = {1.0, 2.0};
    sovereign::Index c1 = model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, vals);

    EXPECT_EQ(model.num_variables(), 2);
    EXPECT_EQ(model.num_constraints(), 1);
    EXPECT_EQ(x1, 0);
    EXPECT_EQ(x2, 1);
    EXPECT_EQ(c1, 0);

    const auto& csc = model.get_csc_matrix();
    EXPECT_TRUE(csc.is_valid());
    EXPECT_EQ(csc.num_rows(), 1);
    EXPECT_EQ(csc.num_cols(), 2);
    EXPECT_EQ(csc.num_nonzeros(), 2);
}

TEST_CASE(LPModelStandardFormConversion) {
    // Original Model:
    // Min 3 x1 + 5 x2
    // s.t. x1 + 2 x2 <= 14  (Row 0, LessEqual -> add slack s1 >= 0)
    //      3 x1 - x2 >= 2   (Row 1, GreaterEqual -> add surplus s2 >= 0)
    //      x1 + x2 == 5     (Row 2, Equal)
    sovereign::lp::LPModel model("original_lp");
    sovereign::Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    sovereign::Index x2 = model.add_variable("x2", 0.0, sovereign::INFINITY_VAL, 5.0);

    std::vector<sovereign::Index> cols = {x1, x2};
    std::vector<double> v1 = {1.0, 2.0};
    std::vector<double> v2 = {3.0, -1.0};
    std::vector<double> v3 = {1.0, 1.0};

    model.add_constraint("c1", sovereign::lp::ConstraintSense::LessEqual, 14.0, cols, v1);
    model.add_constraint("c2", sovereign::lp::ConstraintSense::GreaterEqual, 2.0, cols, v2);
    model.add_constraint("c3", sovereign::lp::ConstraintSense::Equal, 5.0, cols, v3);

    auto std_model = model.convert_to_standard_form();

    // Standard model should have 3 rows and 4 variables (x1, x2, slack_c1, surplus_c2)
    EXPECT_EQ(std_model.num_constraints(), 3);
    EXPECT_EQ(std_model.num_variables(), 4);

    auto val_res = sovereign::lp::LPValidator::validate(std_model);
    EXPECT_TRUE(val_res.is_valid);
}
