#include "test_framework.hpp"
#include "sovereign/lp/mps_parser.hpp"
#include "sovereign/lp/lp_validator.hpp"
#include <sstream>

TEST_CASE(MPSParserStandardSampleLP) {
    std::string mps_data = R"(
NAME          TESTLP
ROWS
 N  COST
 L  ROW1
 G  ROW2
 E  ROW3
COLUMNS
    X1        COST      3.0   ROW1      1.0
    X1        ROW2      2.0
    X2        COST      5.0   ROW1      2.0
    X2        ROW3      1.0
RHS
    RHS1      ROW1     14.0   ROW2      4.0
    RHS1      ROW3      5.0
BOUNDS
 LO BND1      X1        0.0
 UP BND1      X1       10.0
 LO BND1      X2        0.0
ENDATA
)";

    std::istringstream iss(mps_data);
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, model, "string_stream");
    EXPECT_STATUS_OK(status);

    EXPECT_EQ(model.name(), "TESTLP");
    EXPECT_EQ(model.num_variables(), 2);
    EXPECT_EQ(model.num_constraints(), 3);

    auto val_res = sovereign::lp::LPValidator::validate(model);
    EXPECT_TRUE(val_res.is_valid);
}

TEST_CASE(MPSParserInvalidSyntaxReporting) {
    std::string bad_mps = R"(
NAME          BADLP
ROWS
 X  INVALID_ROW_TYPE
COLUMNS
    X1        COST      3.0
ENDATA
)";

    std::istringstream iss(bad_mps);
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, model, "bad_stream");
    EXPECT_FALSE(sovereign::is_ok(status));
    EXPECT_TRUE(!parser.last_error().empty());
    EXPECT_EQ(parser.error_line(), 4);
}
