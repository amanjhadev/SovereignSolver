#include "sovereign/industrial/power_dispatch.hpp"

namespace sovereign::industrial {

PowerDispatchBuilder::PowerDispatchBuilder(std::string name)
    : name_(std::move(name)) {}

void PowerDispatchBuilder::add_generator(const GeneratorUnit& unit) {
    generators_.push_back(unit);
}

qp::QPModel PowerDispatchBuilder::build_economic_dispatch_qp(Float64 total_demand_mw) {
    qp::QPModel qp(name_, ObjectiveSense::Minimize);

    std::vector<Index> gen_indices;
    for (const auto& gen : generators_) {
        Index idx = qp.add_variable(gen.name, gen.p_min, gen.p_max, gen.cost_b);
        qp.add_quadratic_term(idx, idx, 2.0 * gen.cost_a);
        gen_indices.push_back(idx);
    }

    // Power balance constraint: sum(P_g) == total_demand_mw
    std::vector<Float64> ones(gen_indices.size(), 1.0);
    qp.add_constraint("power_balance", lp::ConstraintSense::Equal, total_demand_mw, gen_indices, ones);

    return qp;
}

milp::MILPModel PowerDispatchBuilder::build_unit_commitment_milp(Float64 total_demand_mw, Float64 spinning_reserve_mw) {
    milp::MILPModel milp(name_, ObjectiveSense::Minimize);

    std::vector<Index> p_indices;
    std::vector<Index> u_indices;

    for (const auto& gen : generators_) {
        Index p_idx = milp.add_variable("P_" + gen.name, 0.0, gen.p_max, gen.cost_b, VariableType::Continuous);
        Index u_idx = milp.add_variable("u_" + gen.name, 0.0, 1.0, gen.cost_c + gen.startup_cost, VariableType::Binary);

        p_indices.push_back(p_idx);
        u_indices.push_back(u_idx);

        // Capacity bounds dependent on status: p_min * u_g <= P_g <= p_max * u_g
        std::vector<Index> c1_cols = {p_idx, u_idx};
        std::vector<Float64> c1_vals = {1.0, -gen.p_max};
        milp.add_constraint("cap_max_" + gen.name, lp::ConstraintSense::LessEqual, 0.0, c1_cols, c1_vals);

        std::vector<Index> c2_cols = {p_idx, u_idx};
        std::vector<Float64> c2_vals = {1.0, -gen.p_min};
        milp.add_constraint("cap_min_" + gen.name, lp::ConstraintSense::GreaterEqual, 0.0, c2_cols, c2_vals);
    }

    // Power demand balance: sum(P_g) >= total_demand_mw
    std::vector<Float64> p_ones(p_indices.size(), 1.0);
    milp.add_constraint("demand_balance", lp::ConstraintSense::GreaterEqual, total_demand_mw, p_indices, p_ones);

    // Spinning reserve requirement: sum(p_max * u_g) >= total_demand_mw + spinning_reserve_mw
    if (spinning_reserve_mw > 0.0) {
        std::vector<Float64> p_maxs;
        for (const auto& gen : generators_) {
            p_maxs.push_back(gen.p_max);
        }
        milp.add_constraint("spinning_reserve", lp::ConstraintSense::GreaterEqual, total_demand_mw + spinning_reserve_mw, u_indices, p_maxs);
    }

    return milp;
}

qp::QPModel PowerDispatchBuilder::create_ieee14_economic_dispatch() {
    // IEEE 14-bus System Generator Data (5 generators, 259 MW total demand)
    PowerDispatchBuilder builder("IEEE14_Economic_Dispatch");

    builder.add_generator({"Gen1_Slack", 10.0, 150.0, 0.0015, 18.0, 100.0, 200.0});
    builder.add_generator({"Gen2_PV", 10.0, 80.0, 0.0020, 22.0, 80.0, 150.0});
    builder.add_generator({"Gen3_PV", 10.0, 50.0, 0.0030, 25.0, 60.0, 100.0});
    builder.add_generator({"Gen6_PV", 5.0, 30.0, 0.0040, 30.0, 40.0, 80.0});
    builder.add_generator({"Gen8_PV", 5.0, 30.0, 0.0045, 32.0, 40.0, 80.0});

    return builder.build_economic_dispatch_qp(259.0);
}

milp::MILPModel PowerDispatchBuilder::create_ieee30_unit_commitment() {
    // IEEE 30-bus System Generator Data (6 generators, 283.4 MW demand + 15 MW reserve)
    PowerDispatchBuilder builder("IEEE30_Unit_Commitment");

    builder.add_generator({"G1", 50.0, 200.0, 0.00375, 2.0, 0.0, 100.0});
    builder.add_generator({"G2", 20.0, 80.0, 0.01750, 1.75, 0.0, 200.0});
    builder.add_generator({"G3", 15.0, 50.0, 0.06250, 1.0, 0.0, 300.0});
    builder.add_generator({"G4", 10.0, 35.0, 0.00834, 3.25, 0.0, 400.0});
    builder.add_generator({"G5", 10.0, 30.0, 0.02500, 3.0, 0.0, 500.0});
    builder.add_generator({"G6", 12.0, 40.0, 0.02500, 3.0, 0.0, 500.0});

    return builder.build_unit_commitment_milp(283.4, 15.0);
}

} // namespace sovereign::industrial
