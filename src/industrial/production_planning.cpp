#include "sovereign/industrial/production_planning.hpp"

namespace sovereign::industrial {

ProductionPlanningBuilder::ProductionPlanningBuilder(std::string name, std::size_t num_periods)
    : name_(std::move(name)), num_periods_(num_periods) {
    capacities_.assign(num_periods, 1e20);
}

void ProductionPlanningBuilder::add_product(const ProductDemand& product) {
    products_.push_back(product);
}

void ProductionPlanningBuilder::set_period_capacity(std::size_t period, Float64 max_capacity) {
    if (period < capacities_.size()) {
        capacities_[period] = max_capacity;
    }
}

milp::MILPModel ProductionPlanningBuilder::build() {
    milp::MILPModel model(name_, ObjectiveSense::Minimize);
    std::vector<std::vector<Index>> inv_indices(products_.size(), std::vector<Index>(num_periods_));

    for (std::size_t t = 0; t < num_periods_; ++t) {
        std::vector<Index> cap_p_indices;
        std::vector<Float64> cap_coeffs;

        for (std::size_t p = 0; p < products_.size(); ++p) {
            const auto& prod = products_[p];
            Float64 d_t = (t < prod.period_demand.size()) ? prod.period_demand[t] : 0.0;

            std::string p_name = "P_" + prod.name + "_t" + std::to_string(t);
            std::string i_name = "I_" + prod.name + "_t" + std::to_string(t);

            Index p_idx = model.add_variable(p_name, 0.0, 1e19, prod.production_cost, VariableType::Continuous);
            Index i_idx = model.add_variable(i_name, 0.0, 1e19, prod.holding_cost, VariableType::Continuous);

            inv_indices[p][t] = i_idx;

            cap_p_indices.push_back(p_idx);
            cap_coeffs.push_back(prod.capacity_required);

            // Inventory balance: I_{t-1} + P_t - I_t = D_t
            if (t == 0) {
                // P_0 - I_0 = D_0
                std::vector<Index> b_cols = {p_idx, i_idx};
                std::vector<Float64> b_vals = {1.0, -1.0};
                model.add_constraint("inv_bal_" + prod.name + "_t0", lp::ConstraintSense::Equal, d_t, b_cols, b_vals);
            } else {
                Index prev_i_idx = inv_indices[p][t - 1];
                // I_{t-1} + P_t - I_t = D_t
                std::vector<Index> b_cols = {prev_i_idx, p_idx, i_idx};
                std::vector<Float64> b_vals = {1.0, 1.0, -1.0};
                model.add_constraint("inv_bal_" + prod.name + "_t" + std::to_string(t),
                                     lp::ConstraintSense::Equal, d_t,
                                     b_cols, b_vals);
            }
        }

        // Capacity constraint for period t: sum(cap_req * P_{p,t}) <= Capacity_t
        if (capacities_[t] < 1e19) {
            model.add_constraint("capacity_t" + std::to_string(t), lp::ConstraintSense::LessEqual, capacities_[t], cap_p_indices, cap_coeffs);
        }
    }

    return model;
}

} // namespace sovereign::industrial
