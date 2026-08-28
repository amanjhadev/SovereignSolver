#ifndef SOVEREIGN_MILP_FEASIBILITY_PUMP_HPP
#define SOVEREIGN_MILP_FEASIBILITY_PUMP_HPP

#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_types.hpp"
#include <vector>
#include <optional>

namespace sovereign::milp {

struct HeuristicResult {
    bool found_solution{false};
    Float64 objective_value{0.0};
    std::vector<Float64> solution;
};

class FeasibilityPumpHeuristic {
private:
    Int32 max_iterations_{30};
    Float64 tol_integrality_{1e-6};

public:
    FeasibilityPumpHeuristic() = default;
    explicit FeasibilityPumpHeuristic(Int32 max_iterations, Float64 tol_integrality = 1e-6);

    // Runs Feasibility Pump with explicit L1 distance auxiliary variables
    HeuristicResult run(const MILPModel& model, std::span<const Float64> root_lp_sol) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_FEASIBILITY_PUMP_HPP
