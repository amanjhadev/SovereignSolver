#ifndef SOVEREIGN_MILP_RINS_HEURISTIC_HPP
#define SOVEREIGN_MILP_RINS_HEURISTIC_HPP

#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/feasibility_pump.hpp"
#include <vector>

namespace sovereign::milp {

class RINSHeuristic {
private:
    Int64 sub_node_limit_{30};
    Float64 tol_integrality_{1e-6};

public:
    RINSHeuristic() = default;
    explicit RINSHeuristic(Int64 sub_node_limit, Float64 tol_integrality = 1e-6);

    // Runs Relaxation-Induced Neighborhood Search (RINS)
    HeuristicResult run(
        const MILPModel& model,
        std::span<const Float64> incumbent_sol,
        std::span<const Float64> current_lp_sol
    ) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_RINS_HEURISTIC_HPP
