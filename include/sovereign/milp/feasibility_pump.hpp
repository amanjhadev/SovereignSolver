#ifndef SOVEREIGN_MILP_FEASIBILITY_PUMP_HPP
#define SOVEREIGN_MILP_FEASIBILITY_PUMP_HPP

#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_types.hpp"
#include "sovereign/lp/basis.hpp"
#include <vector>
#include <optional>
#include <span>

namespace sovereign::milp {

struct HeuristicResult {
    bool found_solution{false};
    Float64 objective_value{0.0};
    std::vector<Float64> solution;
};

struct FeasibilityPumpTelemetry {
    Int32 iterations_run{0};
    Int64 auxiliary_vars{0};
    Int64 auxiliary_constraints{0};
    Int64 auxiliary_nnz{0};
    Int32 warm_start_attempts{0};
    Int32 warm_start_accepted{0};
    Int32 warm_start_rejected{0};
    Int64 total_lp_iterations{0};
    double total_lp_solve_time_sec{0.0};
    double total_obj_update_time_sec{0.0};
    Float64 best_l1_distance{INFINITY_VAL};
    Float64 max_constraint_violation{0.0};
    Int32 cycle_detections{0};
    bool found_incumbent{false};
    Float64 incumbent_objective{INFINITY_VAL};
};

class FeasibilityPumpHeuristic {
private:
    Int32 max_iterations_{30};
    Float64 tol_integrality_{1e-6};

public:
    FeasibilityPumpHeuristic() = default;
    explicit FeasibilityPumpHeuristic(Int32 max_iterations, Float64 tol_integrality = 1e-6);

    // Runs Scalable Feasibility Pump (In-Place for binary variables, distance-vars for general integers)
    HeuristicResult run(const MILPModel& model, std::span<const Float64> root_lp_sol) const;

    // Detailed diagnostic run method returning telemetry
    HeuristicResult run_with_telemetry(
        const MILPModel& model,
        std::span<const Float64> root_lp_sol,
        FeasibilityPumpTelemetry& telemetry
    ) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_FEASIBILITY_PUMP_HPP
