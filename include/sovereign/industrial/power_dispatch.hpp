#ifndef SOVEREIGN_INDUSTRIAL_POWER_DISPATCH_HPP
#define SOVEREIGN_INDUSTRIAL_POWER_DISPATCH_HPP

#include "sovereign/qp/qp_model.hpp"
#include "sovereign/milp/milp_model.hpp"
#include <string>
#include <vector>

namespace sovereign::industrial {

struct GeneratorUnit {
    std::string name;
    Float64 p_min{0.0};          // MW
    Float64 p_max{100.0};        // MW
    Float64 cost_a{0.001};       // quadratic fuel cost coefficient ($/MW^2)
    Float64 cost_b{20.0};        // linear fuel cost coefficient ($/MW)
    Float64 cost_c{100.0};       // no-load cost ($)
    Float64 startup_cost{500.0}; // startup cost ($)
};

class PowerDispatchBuilder {
public:
    explicit PowerDispatchBuilder(std::string name);

    void add_generator(const GeneratorUnit& unit);

    // Build Economic Dispatch QP model (Continuous convex QP)
    qp::QPModel build_economic_dispatch_qp(Float64 total_demand_mw);

    // Build Unit Commitment MILP model (Binary status u_g in {0, 1})
    milp::MILPModel build_unit_commitment_milp(Float64 total_demand_mw, Float64 spinning_reserve_mw = 0.0);

    // Built-in benchmark helper: IEEE 14-bus Economic Dispatch QP
    static qp::QPModel create_ieee14_economic_dispatch();

    // Built-in benchmark helper: IEEE 30-bus Unit Commitment MILP
    static milp::MILPModel create_ieee30_unit_commitment();

private:
    std::string name_;
    std::vector<GeneratorUnit> generators_;
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_POWER_DISPATCH_HPP
