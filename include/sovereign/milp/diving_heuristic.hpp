#ifndef SOVEREIGN_MILP_DIVING_HEURISTIC_HPP
#define SOVEREIGN_MILP_DIVING_HEURISTIC_HPP

#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/feasibility_pump.hpp"
#include <vector>

namespace sovereign::milp {

class DivingHeuristic {
private:
    Int32 max_depth_{20};
    Float64 tol_integrality_{1e-6};

public:
    DivingHeuristic() = default;
    explicit DivingHeuristic(Int32 max_depth, Float64 tol_integrality = 1e-6);

    // Runs Fractional Diving heuristic modifying original integer variable bounds
    HeuristicResult run(const MILPModel& model, const lp::LPModel& current_node_lp) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_DIVING_HEURISTIC_HPP
