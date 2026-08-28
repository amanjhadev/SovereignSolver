#include "sovereign/public_solver_api.hpp"
#include "sovereign/test/test_framework.hpp"
#include <iostream>
#include <fstream>

using namespace sovereign;
using namespace sovereign::industrial;

TEST_CASE(IndustrialBlendingModelHaverly) {
    auto model = BlendingModelBuilder::create_haverly_case1();
    EXPECT_EQ(model.num_variables(), 3);
    EXPECT_GE(model.num_constraints(), 3);

    milp::MILPConfig config;
    config.num_threads = 2;
    config.parallel_mode = milp::ParallelMode::Deterministic;

    milp::ParallelMILPSolver solver(config);
    milp::MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(milp::MILPStatus::Optimal));
    EXPECT_NEAR(res.objective_value, 900.0, 10.0);
}

TEST_CASE(IndustrialPowerDispatchIEEE14) {
    auto qp_model = PowerDispatchBuilder::create_ieee14_economic_dispatch();
    EXPECT_EQ(qp_model.num_variables(), 5);
    EXPECT_EQ(qp_model.num_constraints(), 1);

    qp::QPSolver qp_solver;
    qp::QPResult qp_res = qp_solver.solve(qp_model);

    EXPECT_EQ(static_cast<int>(qp_res.status), static_cast<int>(SolverStatus::Optimal));
    EXPECT_GE(qp_res.objective_value, 5000.0);
    EXPECT_NEAR(qp_res.primal_residual, 0.0, 1e-4);
}

TEST_CASE(IndustrialPowerDispatchIEEE30UnitCommitment) {
    auto uc_model = PowerDispatchBuilder::create_ieee30_unit_commitment();
    EXPECT_EQ(uc_model.num_variables(), 12); // 6 continuous P_g + 6 binary u_g

    milp::MILPConfig config;
    config.num_threads = 2;

    milp::ParallelMILPSolver solver(config);
    milp::MILPResult res = solver.solve(uc_model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(milp::MILPStatus::Optimal));
    EXPECT_GE(res.objective_value, 0.0);
}

TEST_CASE(IndustrialProductionPlanningMultiPeriod) {
    ProductionPlanningBuilder builder("ProdPlan3P", 3);
    builder.add_product({"ProductA", 10.0, 2.0, 1.0, {100.0, 150.0, 200.0}});
    builder.set_period_capacity(0, 120.0);
    builder.set_period_capacity(1, 120.0);
    builder.set_period_capacity(2, 250.0);

    milp::MILPModel model = builder.build();
    EXPECT_EQ(model.num_variables(), 6); // 3 P_t + 3 I_t

    milp::MILPSolver solver;
    milp::MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(milp::MILPStatus::Optimal));
    EXPECT_GE(res.objective_value, 4500.0);
}

TEST_CASE(IndustrialSupplyChainFacilityLocation) {
    SupplyChainBuilder builder("FacilityLocation");
    builder.add_facility({"DC1", 1000.0, 200.0});
    builder.add_facility({"DC2", 1500.0, 300.0});
    builder.add_customer({"CustA", 100.0});
    builder.add_customer({"CustB", 150.0});

    builder.set_transport_cost(0, 0, 5.0);
    builder.set_transport_cost(0, 1, 8.0);
    builder.set_transport_cost(1, 0, 7.0);
    builder.set_transport_cost(1, 1, 4.0);

    milp::MILPModel model = builder.build();
    EXPECT_EQ(model.num_variables(), 6); // 2 y_f + 4 x_{f,c}

    milp::MILPSolver solver;
    milp::MILPResult res = solver.solve(model);

    EXPECT_EQ(static_cast<int>(res.status), static_cast<int>(milp::MILPStatus::Optimal));
    EXPECT_GE(res.objective_value, 0.0);
}

TEST_CASE(IndustrialDiagnosticsAndScaling) {
    lp::LPModel model("DiagTest", ObjectiveSense::Minimize);
    model.add_variable("x1", 10.0, 5.0, 1.0); // Bound clash: lower=10 > upper=5
    model.add_variable("x2", 0.0, 100.0, 1.0);
    model.add_constraint("c1", lp::ConstraintSense::LessEqual, 10.0, {0, 1}, {1e10, 1e-2}); // Extreme dynamic range

    DiagnosticReport report = IndustrialDiagnostics::diagnose(model);
    EXPECT_TRUE(report.bound_clashes > 0);
    EXPECT_FALSE(report.is_numerically_well_conditioned);

    // Scaling test
    lp::LPModel scale_model("ScaleTest", ObjectiveSense::Minimize);
    scale_model.add_variable("x1", 0.0, 10.0, 100.0);
    scale_model.add_variable("x2", 0.0, 10.0, 1.0);
    scale_model.add_constraint("c1", lp::ConstraintSense::LessEqual, 1000.0, {0, 1}, {100.0, 0.01});

    ScalingFactors factors = IndustrialScaling::scale_model(scale_model, ScalingPolicy::RuizEquilibration, 2);
    EXPECT_EQ(factors.row_scales.size(), 1);
    EXPECT_EQ(factors.col_scales.size(), 2);
}

TEST_CASE(IndustrialSolverSelectorAndRunReport) {
    lp::LPModel lp_model("ContinuousLP", ObjectiveSense::Minimize);
    lp_model.add_variable("x1", 0.0, 1.0, 1.0);
    lp_model.add_constraint("c1", lp::ConstraintSense::LessEqual, 0.5, {0}, {1.0});

    SelectionResult sel_lp = SolverSelector::select(lp_model);
    EXPECT_EQ(static_cast<int>(sel_lp.engine), static_cast<int>(SelectedEngine::DualSimplex));

    milp::MILPModel milp_model("DiscreteMILP", ObjectiveSense::Minimize);
    milp_model.add_variable("b1", 0.0, 1.0, 1.0, VariableType::Binary);

    SelectionResult sel_milp = SolverSelector::select(milp_model);
    EXPECT_EQ(static_cast<int>(sel_milp.engine), static_cast<int>(SelectedEngine::ParallelMILP));

    RunTelemetry telemetry;
    telemetry.model_name = "TestReport";
    telemetry.solver_engine = "Dual Simplex";
    telemetry.dataset_category = "Standard Benchmark";
    telemetry.scalability_tier = "Tier 1";
    telemetry.variables = 10;
    telemetry.constraints = 5;
    telemetry.runtime_ms = 1.234;

    std::string json_str = RunReport::to_json(telemetry);
    EXPECT_TRUE(json_str.find("TestReport") != std::string::npos);

    std::string md_str = RunReport::to_markdown(telemetry);
    EXPECT_TRUE(md_str.find("Dual Simplex") != std::string::npos);
}

TEST_CASE(IndustrialLPWriterExport) {
    lp::LPModel model("ExportModel", ObjectiveSense::Minimize);
    model.add_variable("x1", 0.0, 10.0, 5.0);
    model.add_variable("x2", 0.0, 20.0, 8.0);
    model.add_constraint("c1", lp::ConstraintSense::LessEqual, 15.0, {0, 1}, {1.0, 2.0});

    bool mps_ok = lp::LPWriter::write_mps(model, "test_export.mps");
    EXPECT_TRUE(mps_ok);

    bool lp_ok = lp::LPWriter::write_lp(model, "test_export.lp");
    EXPECT_TRUE(lp_ok);
}

TEST_CASE(IndustrialStressTestGenerator) {
    auto lp_model = StressTestGenerator::generate_sparse_lp("StressLP", 50, 20, 0.1);
    EXPECT_EQ(lp_model.num_variables(), 50);
    EXPECT_EQ(lp_model.num_constraints(), 20);

    auto milp_model = StressTestGenerator::generate_sparse_milp("StressMILP", 50, 20, 0.2, 0.1);
    EXPECT_EQ(milp_model.num_variables(), 50);
    EXPECT_EQ(milp_model.num_constraints(), 20);
}
