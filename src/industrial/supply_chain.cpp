#include "sovereign/industrial/supply_chain.hpp"

namespace sovereign::industrial {

SupplyChainBuilder::SupplyChainBuilder(std::string name)
    : name_(std::move(name)) {}

void SupplyChainBuilder::add_facility(const FacilityLocation& fac) {
    facilities_.push_back(fac);
    for (auto& row : transport_costs_) {
        row.push_back(10.0);
    }
    transport_costs_.emplace_back(customers_.size(), 10.0);
}

void SupplyChainBuilder::add_customer(const CustomerDemand& cust) {
    customers_.push_back(cust);
    for (auto& row : transport_costs_) {
        row.push_back(10.0);
    }
}

void SupplyChainBuilder::set_transport_cost(std::size_t f_idx, std::size_t c_idx, Float64 unit_cost) {
    if (f_idx < transport_costs_.size() && c_idx < transport_costs_[f_idx].size()) {
        transport_costs_[f_idx][c_idx] = unit_cost;
    }
}

milp::MILPModel SupplyChainBuilder::build() {
    milp::MILPModel model(name_, ObjectiveSense::Minimize);

    std::vector<Index> y_indices;
    for (const auto& fac : facilities_) {
        Index y_idx = model.add_variable("y_" + fac.name, 0.0, 1.0, fac.fixed_cost, VariableType::Binary);
        y_indices.push_back(y_idx);
    }

    std::vector<std::vector<Index>> x_indices(facilities_.size(), std::vector<Index>(customers_.size()));

    for (std::size_t f = 0; f < facilities_.size(); ++f) {
        std::vector<Index> f_x_indices;
        std::vector<Float64> f_x_coeffs;

        for (std::size_t c = 0; c < customers_.size(); ++c) {
            Float64 t_cost = (c < transport_costs_[f].size()) ? transport_costs_[f][c] : 10.0;
            std::string x_name = "x_" + facilities_[f].name + "_" + customers_[c].name;
            Index x_idx = model.add_variable(x_name, 0.0, 1e19, t_cost, VariableType::Continuous);
            x_indices[f][c] = x_idx;

            f_x_indices.push_back(x_idx);
            f_x_coeffs.push_back(1.0);
        }

        // Capacity constraint: sum_c x_{f,c} - capacity * y_f <= 0
        f_x_indices.push_back(y_indices[f]);
        f_x_coeffs.push_back(-facilities_[f].capacity);
        model.add_constraint("cap_" + facilities_[f].name, lp::ConstraintSense::LessEqual, 0.0, f_x_indices, f_x_coeffs);
    }

    // Customer demand constraint: sum_f x_{f,c} == demand_c
    for (std::size_t c = 0; c < customers_.size(); ++c) {
        std::vector<Index> c_x_indices;
        std::vector<Float64> c_ones;
        for (std::size_t f = 0; f < facilities_.size(); ++f) {
            c_x_indices.push_back(x_indices[f][c]);
            c_ones.push_back(1.0);
        }
        model.add_constraint("demand_" + customers_[c].name, lp::ConstraintSense::Equal, customers_[c].demand, c_x_indices, c_ones);
    }

    return model;
}

} // namespace sovereign::industrial
