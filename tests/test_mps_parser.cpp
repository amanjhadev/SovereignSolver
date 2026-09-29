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

TEST_CASE(MPSParserIntegerMarkersINTORG) {
    std::string mps_data = R"(
NAME          INTMARKLM
ROWS
 N  COST
 L  C1
COLUMNS
    MARK0000  'MARKER'                 'INTORG'
    X1        COST      1.0   C1        1.0
    MARK0001  'MARKER'                 'INTEND'
    X2        COST      2.0   C1        1.0
RHS
    RHS1      C1       10.0
BOUNDS
 FX BND       X1        1.0
ENDATA
)";

    std::istringstream iss(mps_data);
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, model, "int_marker_stream");
    EXPECT_STATUS_OK(status);

    EXPECT_EQ(model.num_variables(), 2);
    EXPECT_EQ(model.num_constraints(), 1);

    // X1 should be Binary
    const auto& var1 = model.get_variable(0);
    EXPECT_EQ(var1.name, "X1");
    EXPECT_EQ(static_cast<int>(var1.type), static_cast<int>(sovereign::VariableType::Binary));

    // X2 should be Continuous
    const auto& var2 = model.get_variable(1);
    EXPECT_EQ(var2.name, "X2");
    EXPECT_EQ(static_cast<int>(var2.type), static_cast<int>(sovereign::VariableType::Continuous));
}

TEST_CASE(MPSParserObjectiveSenseMaximizeTwoLine) {
    std::string mps_data = R"(
NAME          TEST_MAX
OBJSENSE
  MAX
ROWS
 N  COST
 L  ROW1
COLUMNS
    X1        COST      5.0   ROW1      1.0
RHS
    RHS1      ROW1      1.0
BOUNDS
 BV BND       X1
ENDATA
)";

    std::istringstream iss(mps_data);
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, model, "objsense_max_stream");
    EXPECT_STATUS_OK(status);
    EXPECT_EQ(model.name(), "TEST_MAX");
    EXPECT_EQ(static_cast<int>(model.objective_sense()), static_cast<int>(sovereign::ObjectiveSense::Maximize));
}

TEST_CASE(MPSParserObjectiveSenseMaximizeSingleLine) {
    std::string mps_data = R"(
NAME          TEST_MAX_INLINE
OBJSENSE MAX
ROWS
 N  COST
 L  ROW1
COLUMNS
    X1        COST      5.0   ROW1      1.0
RHS
    RHS1      ROW1      1.0
BOUNDS
 BV BND       X1
ENDATA
)";

    std::istringstream iss(mps_data);
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, model, "objsense_inline_stream");
    EXPECT_STATUS_OK(status);
    EXPECT_EQ(static_cast<int>(model.objective_sense()), static_cast<int>(sovereign::ObjectiveSense::Maximize));
}

TEST_CASE(MPSParserObjectiveSenseMinimize) {
    std::string mps_data = R"(
NAME          TEST_MIN
OBJSENSE
  MIN
ROWS
 N  COST
 L  ROW1
COLUMNS
    X1        COST      5.0   ROW1      1.0
RHS
    RHS1      ROW1      1.0
BOUNDS
 BV BND       X1
ENDATA
)";

    std::istringstream iss(mps_data);
    sovereign::lp::LPModel model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, model, "objsense_min_stream");
    EXPECT_STATUS_OK(status);
    EXPECT_EQ(static_cast<int>(model.objective_sense()), static_cast<int>(sovereign::ObjectiveSense::Minimize));
}

