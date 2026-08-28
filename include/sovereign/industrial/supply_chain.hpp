#ifndef SOVEREIGN_INDUSTRIAL_SUPPLY_CHAIN_HPP
#define SOVEREIGN_INDUSTRIAL_SUPPLY_CHAIN_HPP

#include "sovereign/milp/milp_model.hpp"
#include <string>
#include <vector>

namespace sovereign::industrial {

struct FacilityLocation {
    std::string name;
    Float64 fixed_cost{1000.0};
    Float64 capacity{500.0};
};

struct CustomerDemand {
    std::string name;
    Float64 demand{50.0};
};

class SupplyChainBuilder {
public:
    explicit SupplyChainBuilder(std::string name);

    void add_facility(const FacilityLocation& facility);
    void add_customer(const CustomerDemand& customer);
    void set_transport_cost(std::size_t facility_idx, std::size_t customer_idx, Float64 unit_cost);

    milp::MILPModel build();

private:
    std::string name_;
    std::vector<FacilityLocation> facilities_;
    std::vector<CustomerDemand> customers_;
    std::vector<std::vector<Float64>> transport_costs_;
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_SUPPLY_CHAIN_HPP
