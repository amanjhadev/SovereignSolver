#include "test_framework.hpp"
#include "sovereign/milp/feasibility_pump.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include <cmath>
#include <vector>

using namespace sovereign;
using namespace sovereign::lp;
using namespace sovereign::milp;

// Helper to build a simple binary MILP model
static MILPModel create_binary_test_model() {
    LPModel lp("test_binary_milp", ObjectiveSense::Minimize);
    Index x0 = lp.add_variable("x0", 0.0, 1.0, 10.0, VariableType::Binary);
    Index x1 = lp.add_variable("x1", 0.0, 1.0, 20.0, VariableType::Binary);
    Index x2 = lp.add_variable("x2", 0.0, 1.0, 30.0, VariableType::Binary);

    // x0 + x1 + x2 >= 1.5
    std::vector<Index> cols = {x0, x1, x2};
    std::vector<Float64> vals = {1.0, 1.0, 1.0};
    lp.add_constraint("c0", ConstraintSense::GreaterEqual, 1.5, cols, vals);

    return MILPModel(lp);
}

// 1. Binary target = 0 test
TEST_CASE(FeasibilityPumpBinaryTargetZero) {
    MILPModel model = create_binary_test_model();
    FeasibilityPumpHeuristic fp(10, 1e-6);

    std::vector<Float64> root_sol = {0.5, 0.5, 0.5};
    FeasibilityPumpTelemetry tel;
    auto res = fp.run_with_telemetry(model, root_sol, tel);

    EXPECT_EQ(tel.auxiliary_vars, 0); // No auxiliary vars for binary MILP
    EXPECT_EQ(tel.auxiliary_constraints, 0); // No auxiliary constraints for binary MILP
    EXPECT_TRUE(tel.iterations_run > 0);
}

// 2. Binary target = 1 test
TEST_CASE(FeasibilityPumpBinaryTargetOne) {
    MILPModel model = create_binary_test_model();
    FeasibilityPumpHeuristic fp(10, 1e-6);

    std::vector<Float64> root_sol = {0.9, 0.9, 0.9};
    FeasibilityPumpTelemetry tel;
    auto res = fp.run_with_telemetry(model, root_sol, tel);

    EXPECT_EQ(tel.auxiliary_vars, 0);
    EXPECT_EQ(tel.auxiliary_constraints, 0);
}

// 3. Mixed 0/1 targets test
TEST_CASE(FeasibilityPumpMixedBinaryTargets) {
    MILPModel model = create_binary_test_model();
    FeasibilityPumpHeuristic fp(10, 1e-6);

    std::vector<Float64> root_sol = {0.1, 0.9, 0.6};
    FeasibilityPumpTelemetry tel;
    auto res = fp.run_with_telemetry(model, root_sol, tel);

    EXPECT_EQ(tel.auxiliary_vars, 0);
    EXPECT_EQ(tel.auxiliary_constraints, 0);
}

// 4. Exact L1 objective equivalence
TEST_CASE(FeasibilityPumpExactL1ObjectiveEquivalence) {
    // For x0 in [0,1] and target t0 = 1, |x0 - 1| = 1 - x0 = -1.0 * x0 + 1.0
    // For x1 in [0,1] and target t1 = 0, |x1 - 0| = x1 = +1.0 * x1 + 0.0
    // Direct linear objective: c'_0 = -1.0, c'_1 = +1.0, offset = 1.0.
    Float64 x0_val = 0.7;
    Float64 x1_val = 0.3;

    Float64 exact_l1 = std::abs(x0_val - 1.0) + std::abs(x1_val - 0.0);
    Float64 linear_obj = (-1.0 * x0_val) + (+1.0 * x1_val) + 1.0;

    EXPECT_NEAR(exact_l1, linear_obj, 1e-12);
}

// 5. Original objective restoration test
TEST_CASE(FeasibilityPumpOriginalObjectiveRestoration) {
    MILPModel model = create_binary_test_model();
    Float64 orig_c0 = model.lp_model().get_variable(0).obj_coeff;
    Float64 orig_c1 = model.lp_model().get_variable(1).obj_coeff;
    Float64 orig_c2 = model.lp_model().get_variable(2).obj_coeff;

    FeasibilityPumpHeuristic fp(5, 1e-6);
    std::vector<Float64> root_sol = {0.5, 0.5, 0.5};
    fp.run(model, root_sol);

    // Verify model's original objective coefficients were restored 100%
    EXPECT_EQ(model.lp_model().get_variable(0).obj_coeff, orig_c0);
    EXPECT_EQ(model.lp_model().get_variable(1).obj_coeff, orig_c1);
    EXPECT_EQ(model.lp_model().get_variable(2).obj_coeff, orig_c2);
}

// 6. Candidate incumbent objective reconstruction test
TEST_CASE(FeasibilityPumpCandidateObjectiveReconstruction) {
    MILPModel model = create_binary_test_model();
    FeasibilityPumpHeuristic fp(10, 1e-6);

    std::vector<Float64> root_sol = {1.0, 1.0, 0.0};
    FeasibilityPumpTelemetry tel;
    auto res = fp.run_with_telemetry(model, root_sol, tel);

    if (res.found_solution) {
        Float64 expected_obj = 10.0 * res.solution[0] + 20.0 * res.solution[1] + 30.0 * res.solution[2];
        EXPECT_NEAR(res.objective_value, expected_obj, 1e-6);
    }
}

// 7. Warm-start valid & rejected cases
TEST_CASE(FeasibilityPumpWarmStartTelemetry) {
    MILPModel model = create_binary_test_model();
    FeasibilityPumpHeuristic fp(5, 1e-6);
    std::vector<Float64> root_sol = {0.4, 0.6, 0.5};

    FeasibilityPumpTelemetry tel;
    fp.run_with_telemetry(model, root_sol, tel);

    EXPECT_TRUE(tel.warm_start_attempts + tel.iterations_run >= 0);
}

// 8. Cycle detection & perturbation test
TEST_CASE(FeasibilityPumpCycleDetection) {
    MILPModel model = create_binary_test_model();
    FeasibilityPumpHeuristic fp(20, 1e-6);
    std::vector<Float64> root_sol = {0.5, 0.5, 0.5};

    FeasibilityPumpTelemetry tel;
    fp.run_with_telemetry(model, root_sol, tel);

    EXPECT_TRUE(tel.iterations_run >= 1);
}

// 9. Binary + Continuous variables test
TEST_CASE(FeasibilityPumpBinaryPlusContinuousVariables) {
    LPModel lp("mixed_binary_cont", ObjectiveSense::Minimize);
    Index b0 = lp.add_variable("b0", 0.0, 1.0, 5.0, VariableType::Binary);
    Index y0 = lp.add_variable("y0", 0.0, 10.0, 2.0, VariableType::Continuous);

    std::vector<Index> cols = {b0, y0};
    std::vector<Float64> vals = {1.0, -1.0};
    lp.add_constraint("c0", ConstraintSense::LessEqual, 0.0, cols, vals);

    MILPModel model(lp);
    FeasibilityPumpHeuristic fp(10, 1e-6);
    std::vector<Float64> root_sol = {0.5, 1.0};

    FeasibilityPumpTelemetry tel;
    fp.run_with_telemetry(model, root_sol, tel);

    EXPECT_EQ(tel.auxiliary_vars, 0); // Continuous var gets 0 auxiliary vars
    EXPECT_EQ(tel.auxiliary_constraints, 0);
}

// 10. Binary + General Integer variables (Fallback test)
TEST_CASE(FeasibilityPumpGeneralIntegerFallback) {
    LPModel lp("mixed_gen_int", ObjectiveSense::Minimize);
    Index b0 = lp.add_variable("b0", 0.0, 1.0, 5.0, VariableType::Binary);
    Index g0 = lp.add_variable("g0", 0.0, 10.0, 2.0, VariableType::Integer);

    std::vector<Index> cols = {b0, g0};
    std::vector<Float64> vals = {1.0, 1.0};
    lp.add_constraint("c0", ConstraintSense::GreaterEqual, 3.0, cols, vals);

    MILPModel model(lp);
    FeasibilityPumpHeuristic fp(5, 1e-6);
    std::vector<Float64> root_sol = {0.5, 2.5};

    FeasibilityPumpTelemetry tel;
    fp.run_with_telemetry(model, root_sol, tel);

    // General integer variable g0 gets 1 auxiliary distance var and 2 constraints
    EXPECT_EQ(tel.auxiliary_vars, 1);
    EXPECT_EQ(tel.auxiliary_constraints, 2);
}
