#include "test_framework.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include "sovereign/milp/presolve_milp_solver.hpp"
#include "sovereign/milp/cut_pool.hpp"
#include "sovereign/milp/mir_cut.hpp"
#include "sovereign/milp/cover_cut.hpp"
#include "sovereign/milp/feasibility_pump.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/lp/mps_parser.hpp"
#include <vector>
#include <cmath>

using namespace sovereign;
using namespace sovereign::lp;
using namespace sovereign::milp;

TEST_CASE(MILPPureIntegerLP) {
    // min -x1 - 2*x2  s.t.  x1 + x2 <= 4.5, x1, x2 in Z>=0
    // Optimal integer solution: x1 = 0, x2 = 4, obj = -8.0
    MILPModel model("pure_int_lp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -2.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 4.5, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.primal_solution[x1], 0.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 4.0, 1e-3);
    EXPECT_NEAR(res.objective_value, -8.0, 1e-3);
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-3);
}

TEST_CASE(MILPBinaryKnapsack) {
    // max 10*x1 + 15*x2 + 25*x3 + 20*x4
    // s.t. 2*x1 + 3*x2 + 5*x3 + 4*x4 <= 7, x_i in {0, 1}
    // Optimal knapsack value = 35.0 ({x1=1, x3=1} or {x2=1, x4=1})
    MILPModel model("binary_knapsack", ObjectiveSense::Maximize);
    Index x1 = model.add_variable("x1", 0.0, 1.0, 10.0, VariableType::Binary);
    Index x2 = model.add_variable("x2", 0.0, 1.0, 15.0, VariableType::Binary);
    Index x3 = model.add_variable("x3", 0.0, 1.0, 25.0, VariableType::Binary);
    Index x4 = model.add_variable("x4", 0.0, 1.0, 20.0, VariableType::Binary);

    std::vector<Index> cols = {x1, x2, x3, x4};
    std::vector<Float64> vals = {2.0, 3.0, 5.0, 4.0};
    model.add_constraint("weight_limit", ConstraintSense::LessEqual, 7.0, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 35.0, 1e-3);
}

TEST_CASE(MILPMixedIntegerLP) {
    // Continuous x1 >= 0, General Integer x2 in Z>=0
    // min x1 + 2*x2  s.t.  x1 + x2 = 3.5
    // Optimal: x2 = 0, x1 = 3.5, obj = 3.5 + 2*0 = 3.5
    MILPModel model("mixed_int_lp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::Continuous);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::Equal, 3.5, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.primal_solution[x1], 3.5, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 3.5, 1e-3);
}

TEST_CASE(MILPFreeIntegerVariableBranching) {
    // min x1 + x2  s.t.  x1 - 2*x2 <= -0.5, x1 in Integer [-10, 10], x2 in R>=0
    // Optimal integer solution: x1 = -10, x2 = 0.0 -> obj = -10.0
    MILPModel model("free_int_branching", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", -10.0, 10.0, 1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0, VariableType::Continuous);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, -2.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, -0.5, cols, vals);

    MILPConfig config;
    config.enable_cuts = false;
    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.primal_solution[x1], -10.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-3);
    EXPECT_NEAR(res.objective_value, -10.0, 1e-3);
}

TEST_CASE(MILPInfeasibleDetection) {
    // Infeasible MILP model: x1 + x2 <= 1.5, x1 >= 1, x2 >= 1, x1, x2 in Z>=0
    MILPModel model("infeasible_milp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 1.0, 10.0, 1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 1.0, 10.0, 1.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 1.5, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Infeasible));
}

TEST_CASE(MILPRootIntegral) {
    // LP relaxation integral at root: min x1 + x2 s.t. x1 + x2 >= 5, x1, x2 in Z>=0
    // Continuous LP relaxation gives integral solution x1=5, x2=0 -> 0 B&B branches!
    MILPModel model("root_integral", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 1.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 5.0, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_EQ(res.nodes_explored, 0);
    EXPECT_NEAR(res.objective_value, 5.0, 1e-3);
}

TEST_CASE(MILPFractionalRootBranching) {
    // Multi-level branching test: min -x1 - x2  s.t. 2*x1 + x2 <= 5, x1 + 2*x2 <= 5, x1, x2 in Z>=0
    // Continuous root: x1 = 1.667, x2 = 1.667 (fractional!)
    // Integer optimal: x1 = 1, x2 = 2 or x1 = 2, x2 = 1, obj = -3.0
    MILPModel model("multilevel_branching", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -1.0, VariableType::Integer);

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {2.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 5.0, cols1, vals1);

    std::vector<Index> cols2 = {x1, x2};
    std::vector<Float64> vals2 = {1.0, 2.0};
    model.add_constraint("c2", ConstraintSense::LessEqual, 5.0, cols2, vals2);

    MILPConfig config;
    config.enable_cuts = false;
    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_TRUE(res.nodes_explored > 0);
    EXPECT_NEAR(res.objective_value, -3.0, 1e-3);
}

TEST_CASE(MILPBranchingRulesCompare) {
    // Test that both MostFractional and FirstFractional arrive at same optimal solution
    MILPModel model("branch_rules_compare", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -2.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -3.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 3.5, cols, vals);

    MILPConfig cfg1;
    cfg1.branching_rule = BranchingRule::MostFractional;
    MILPSolver solver1(cfg1);
    MILPResult res1 = solver1.solve(model);

    MILPConfig cfg2;
    cfg2.branching_rule = BranchingRule::FirstFractional;
    MILPSolver solver2(cfg2);
    MILPResult res2 = solver2.solve(model);

    EXPECT_EQ(static_cast<int>(res1.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_EQ(static_cast<int>(res2.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res1.objective_value, res2.objective_value, 1e-3);
}

TEST_CASE(MILPIncumbentPruning) {
    // Test node fathoming by bound when an incumbent is established
    MILPModel model("incumbent_pruning", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 5.0, 1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 5.0, 2.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 2.5, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 3.0, 1e-3);
}

TEST_CASE(MILPMinAndMaxSense) {
    // Test Maximize objective sense: max x1 + 2*x2 s.t. x1 + x2 <= 3.5, x1, x2 in Z>=0
    // Optimal: x1 = 0, x2 = 3 -> obj = 6.0
    MILPModel model("max_sense_milp", ObjectiveSense::Maximize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 3.5, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.primal_solution[x2], 3.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 6.0, 1e-3);
}

TEST_CASE(MILPPresolveIntegration) {
    // Presolve + MILP + Postsolve end-to-end integration test
    MILPModel model("presolve_milp_integration", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 2.0, 2.0, 1.0, VariableType::Integer); // Fixed at 2.0
    Index x2 = model.add_variable("x2", 0.0, 10.0, -2.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 6.5, cols, vals);

    PresolveMILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.primal_solution[x1], 2.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 4.0, 1e-3);
    EXPECT_NEAR(res.objective_value, -6.0, 1e-3);
}

TEST_CASE(MILPGlobalOptimalityProof) {
    // Verification of zero MIP gap proof
    MILPModel model("global_optimality_proof", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 5.0, 3.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 5.0, 4.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 2.2, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-3);
}

TEST_CASE(RegressionNoIncumbentNotInfeasible) {
    // Empty B&B queue without an incumbent MUST return Infeasible, NOT Optimal
    MILPModel model("no_incumbent_reg", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 2.0, 2.0, 1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 2.0, 2.0, 1.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 1.0, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Infeasible));
    EXPECT_TRUE(res.status != MILPStatus::Optimal);
}

TEST_CASE(RegressionLPUnboundedToMILPUnresolved) {
    // Unbounded continuous LP relaxation without integer ray returns Unresolved, NOT Unbounded or Optimal
    MILPModel model("unbounded_reg", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, INFINITY_VAL, -1.0, VariableType::Integer);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Unresolved));
    EXPECT_TRUE(res.status != MILPStatus::Unbounded);
}

TEST_CASE(MILPBranchingValidationMin) {
    // Model: min -x1 - x2  s.t. 2*x1 + x2 <= 5, x1 + 2*x2 <= 5, x1, x2 in Z>=0
    // Continuous root: x1 = 1.6667, x2 = 1.6667, obj = -3.3333 (2 fractional vars!)
    // Integer optimal: x1 = 1, x2 = 2 or x1 = 2, x2 = 1, obj = -3.0
    // FORCES B&B: Nodes explored > 0, LP solves > 1, incumbent updated, fathomed by bound!
    MILPModel model("branch_val_min", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -1.0, VariableType::Integer);

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {2.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 5.0, cols1, vals1);

    std::vector<Index> cols2 = {x1, x2};
    std::vector<Float64> vals2 = {1.0, 2.0};
    model.add_constraint("c2", ConstraintSense::LessEqual, 5.0, cols2, vals2);

    MILPConfig config;
    config.enable_cuts = false;
    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_TRUE(res.nodes_explored > 0);
    EXPECT_TRUE(res.lp_solves > 1);
    EXPECT_NEAR(res.objective_value, -3.0, 1e-3);
    EXPECT_NEAR(res.best_bound, -3.0, 1e-3);
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-3);
}

TEST_CASE(MILPBranchingValidationMax) {
    // Model: max 3*x1 + 2*x2  s.t. x1 + 2*x2 <= 6, 2*x1 + x2 <= 8, x1, x2 in Z>=0
    // Continuous root: x1 = 3.3333, x2 = 1.3333, obj = 12.6667 (2 fractional vars!)
    // Integer optimal: x1 = 4, x2 = 0, obj = 12.0
    // FORCES B&B: Nodes explored > 0, LP solves > 1, incumbent updated, fathomed by bound/infeasibility!
    MILPModel model("branch_val_max", ObjectiveSense::Maximize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 3.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, 2.0, VariableType::Integer);

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {1.0, 2.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 6.0, cols1, vals1);

    std::vector<Index> cols2 = {x1, x2};
    std::vector<Float64> vals2 = {2.0, 1.0};
    model.add_constraint("c2", ConstraintSense::LessEqual, 8.0, cols2, vals2);

    MILPConfig config;
    config.enable_cuts = false;
    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_TRUE(res.nodes_explored > 0);
    EXPECT_TRUE(res.lp_solves > 1);
    EXPECT_NEAR(res.primal_solution[x1], 4.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 12.0, 1e-3);
    EXPECT_NEAR(res.best_bound, 12.0, 1e-3);
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-3);
}

TEST_CASE(CutPoolValidityAndFiltering) {
    CutPool pool(1e-4, 0.90);
    std::vector<Float64> sol = {1.5, 2.0};

    Cut invalid_cut;
    invalid_cut.rhs = 1.0;
    invalid_cut.col_indices = {0};
    invalid_cut.coefficients = {NAN}; // Invalid numerical coefficient
    auto val_res = pool.validate_cut(invalid_cut, sol);
    EXPECT_FALSE(val_res.is_valid);

    Cut valid_cut;
    valid_cut.name = "test_c1";
    valid_cut.sense = ConstraintSense::GreaterEqual;
    valid_cut.rhs = 2.0;
    valid_cut.col_indices = {0};
    valid_cut.coefficients = {1.0}; // x0 >= 2.0, violated by x0=1.5
    bool added = pool.add_cut(valid_cut, sol);
    EXPECT_TRUE(added);
    EXPECT_EQ(pool.size(), 1);
}

TEST_CASE(CanonicalMIRSeparation) {
    MILPModel model("mir_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::Integer);
    Index c1 = model.add_variable("c1", 0.0, 10.0, 1.0, VariableType::Continuous);

    std::vector<Index> cols = {x1, c1};
    std::vector<Float64> vals = {2.5, 1.0};
    model.add_constraint("row1", ConstraintSense::LessEqual, 6.2, cols, vals);

    CanonicalMIRCutGenerator mir_gen(1e-6, 1e-4);
    std::vector<Float64> sol = {2.7, 0.0}; // Violates row: 2.5*2.7 = 6.75 > 6.2
    auto cut_opt = mir_gen.separate_row(model, cols, vals, 6.2, sol, 0);
    EXPECT_TRUE(cut_opt.has_value());
}

TEST_CASE(MinimalKnapsackCoverSeparation) {
    MILPModel model("cover_test", ObjectiveSense::Maximize);
    Index x1 = model.add_variable("x1", 0.0, 1.0, 10.0, VariableType::Binary);
    Index x2 = model.add_variable("x2", 0.0, 1.0, 10.0, VariableType::Binary);
    Index x3 = model.add_variable("x3", 0.0, 1.0, 10.0, VariableType::Binary);

    std::vector<Index> cols = {x1, x2, x3};
    std::vector<Float64> vals = {3.0, 4.0, 5.0};
    model.add_constraint("knap", ConstraintSense::LessEqual, 6.0, cols, vals);

    MinimalCoverCutGenerator cover_gen(1e-4);
    std::vector<Float64> sol = {1.0, 1.0, 0.5}; // Violates cover {x1, x2, x3} weight 12 > 6
    auto cut_opt = cover_gen.separate_knapsack_row(model, cols, vals, 6.0, sol, 0);
    EXPECT_TRUE(cut_opt.has_value());
}

TEST_CASE(FeasibilityPumpExecution) {
    MILPModel model("fpump_test", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 5.0, 1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 5.0, 2.0, VariableType::Integer);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {1.0, 1.0};
    model.add_constraint("c1", ConstraintSense::GreaterEqual, 2.5, cols, vals);

    FeasibilityPumpHeuristic fpump(20, 1e-6);
    std::vector<Float64> root_sol = {1.2, 1.3};
    auto heur_res = fpump.run(model, root_sol);
    EXPECT_TRUE(heur_res.found_solution);
    EXPECT_TRUE(heur_res.objective_value >= 3.0);
}

TEST_CASE(Batch8MILPIntegrationWithCuts) {
    MILPModel model("batch8_cuts_milp", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -1.0, VariableType::Integer);

    std::vector<Index> cols1 = {x1, x2};
    std::vector<Float64> vals1 = {2.0, 1.0};
    model.add_constraint("c1", ConstraintSense::LessEqual, 5.0, cols1, vals1);

    std::vector<Index> cols2 = {x1, x2};
    std::vector<Float64> vals2 = {1.0, 2.0};
    model.add_constraint("c2", ConstraintSense::LessEqual, 5.0, cols2, vals2);

    MILPConfig config;
    config.enable_cuts = true;
    config.max_cut_passes = 3;
    config.enable_feasibility_pump = true;
    config.enable_diving = true;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.objective_value, -3.0, 1e-3);
    EXPECT_NEAR(res.best_bound, -3.0, 1e-3);
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-3);
}

TEST_CASE(DualSimplexWarmStartReoptimizationBoundTightening) {
    // LP: min -x1 - x2 s.t. 2*x1 + x2 <= 5, x1 + 2*x2 <= 5, 0 <= x1, x2 <= 10
    // Continuous root optimum: x1 = 5/3 (1.66667), x2 = 5/3, obj = -10/3 (-3.33333)
    LPModel model("root_model");
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -1.0);

    std::vector<Index> c1 = {x1, x2};
    std::vector<Float64> v1 = {2.0, 1.0};
    model.add_constraint("row1", ConstraintSense::LessEqual, 5.0, c1, v1);

    std::vector<Index> c2 = {x1, x2};
    std::vector<Float64> v2 = {1.0, 2.0};
    model.add_constraint("row2", ConstraintSense::LessEqual, 5.0, c2, v2);

    DualSimplexSolver solver;
    SimplexResult root_res = solver.solve(model);
    EXPECT_EQ(static_cast<int>(root_res.status), static_cast<int>(SimplexStatus::Optimal));
    EXPECT_NEAR(root_res.primal_solution[x1], 5.0 / 3.0, 1e-5);
    EXPECT_NEAR(root_res.primal_solution[x2], 5.0 / 3.0, 1e-5);
    EXPECT_NEAR(root_res.objective_value, -10.0 / 3.0, 1e-5);

    // Tighten child bound: x1 <= 1.0 (branch floor)
    LPModel child_model = model;
    child_model.get_variable(x1).upper_bound = 1.0;

    SimplexResult child_res = solver.solve(child_model, root_res.basis);
    EXPECT_EQ(static_cast<int>(child_res.status), static_cast<int>(SimplexStatus::Optimal));
    // Must respect the newly imposed upper bound x1 <= 1.0
    EXPECT_TRUE(child_res.primal_solution[x1] <= 1.0 + 1e-6);
    // At x1 = 1, x2 = (5 - 1)/2 = 2, obj = -3.0
    EXPECT_NEAR(child_res.primal_solution[x1], 1.0, 1e-4);
    EXPECT_NEAR(child_res.primal_solution[x2], 2.0, 1e-4);
    EXPECT_NEAR(child_res.objective_value, -3.0, 1e-4);
}

TEST_CASE(MILPBranchAndBoundConvergenceRegression) {
    // Pure B&B test with all heuristics and cuts disabled to ensure branch-and-bound warm start converges
    MILPModel model("pure_bb_convergence", ObjectiveSense::Minimize);
    Index x1 = model.add_variable("x1", 0.0, 10.0, -1.0, VariableType::Integer);
    Index x2 = model.add_variable("x2", 0.0, 10.0, -1.0, VariableType::Integer);

    std::vector<Index> c1 = {x1, x2};
    std::vector<Float64> v1 = {2.0, 1.0};
    model.add_constraint("row1", ConstraintSense::LessEqual, 5.0, c1, v1);

    std::vector<Index> c2 = {x1, x2};
    std::vector<Float64> v2 = {1.0, 2.0};
    model.add_constraint("row2", ConstraintSense::LessEqual, 5.0, c2, v2);

    MILPConfig config;
    config.enable_cuts = false;
    config.enable_feasibility_pump = false;
    config.enable_diving = false;

    MILPSolver solver(config);
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.objective_value, -3.0, 1e-3);
    EXPECT_NEAR(res.best_bound, -3.0, 1e-3);
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-3);
    // Before fix, this took 23,718 nodes due to warm-start restoring parent fractional state.
    EXPECT_TRUE(res.nodes_explored <= 10);
}

TEST_CASE(MILPObjectiveSenseMaximizationEndToEnd) {
    // MILP: max 5*x1 + 3*x2 s.t. 2*x1 + 2*x2 <= 3, x1, x2 binary
    // Feasible binary solutions:
    // (0, 0) -> obj 0
    // (1, 0) -> obj 5 (feasible: 2 <= 3)
    // (0, 1) -> obj 3 (feasible: 2 <= 3)
    // (1, 1) -> infeasible: 4 > 3
    // Optimum: obj = 5.0 at (1, 0)
    MILPModel model("max_knapsack", ObjectiveSense::Maximize);
    Index x1 = model.add_variable("x1", 0.0, 1.0, 5.0, VariableType::Binary);
    Index x2 = model.add_variable("x2", 0.0, 1.0, 3.0, VariableType::Binary);

    std::vector<Index> cols = {x1, x2};
    std::vector<Float64> vals = {2.0, 2.0};
    model.add_constraint("cap", ConstraintSense::LessEqual, 3.0, cols, vals);

    MILPSolver solver;
    MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(MILPStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 5.0, 1e-4);
    EXPECT_NEAR(res.best_bound, 5.0, 1e-4);
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[x1], 1.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-4);
}

TEST_CASE(MILPMPSParserMaximizationEndToEnd) {
    std::string mps_data = R"(
NAME          MAX_KNAPSACK_MPS
OBJSENSE
  MAX
ROWS
 N  OBJ
 L  CAP
COLUMNS
    MARK0000  'MARKER'                 'INTORG'
    X1        OBJ       5.0   CAP       2.0
    X2        OBJ       3.0   CAP       2.0
    MARK0001  'MARKER'                 'INTEND'
RHS
    RHS1      CAP       3.0
BOUNDS
 BV BND       X1
 BV BND       X2
ENDATA
)";

    std::istringstream iss(mps_data);
    sovereign::lp::LPModel lp_model;
    sovereign::lp::MPSParser parser;

    auto status = parser.parse_stream(iss, lp_model, "max_mps_stream");
    EXPECT_STATUS_OK(status);
    EXPECT_EQ(static_cast<int>(lp_model.objective_sense()), static_cast<int>(sovereign::ObjectiveSense::Maximize));

    sovereign::milp::MILPModel milp_model(lp_model);
    EXPECT_EQ(static_cast<int>(milp_model.objective_sense()), static_cast<int>(sovereign::ObjectiveSense::Maximize));

    sovereign::milp::MILPSolver solver;
    auto res = solver.solve(milp_model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(sovereign::milp::MILPStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 5.0, 1e-4);
    EXPECT_NEAR(res.best_bound, 5.0, 1e-4);
    EXPECT_NEAR(res.mip_gap, 0.0, 1e-4);
}

