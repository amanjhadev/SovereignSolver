#ifndef SOVEREIGN_INDUSTRIAL_PRODUCTION_PLANNING_HPP
#define SOVEREIGN_INDUSTRIAL_PRODUCTION_PLANNING_HPP

#include "sovereign/milp/milp_model.hpp"
#include <string>
#include <vector>

namespace sovereign::industrial {

struct ProductDemand {
    std::string name;
    Float64 production_cost{10.0};
    Float64 holding_cost{2.0};
    Float64 capacity_required{1.0}; // Unit capacity per product unit
    std::vector<Float64> period_demand;
};

class ProductionPlanningBuilder {
public:
    explicit ProductionPlanningBuilder(std::string name, std::size_t num_periods);

    void add_product(const ProductDemand& product);
    void set_period_capacity(std::size_t period, Float64 max_capacity);

    milp::MILPModel build();

private:
    std::string name_;
    std::size_t num_periods_;
    std::vector<ProductDemand> products_;
    std::vector<Float64> capacities_;
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_PRODUCTION_PLANNING_HPP
