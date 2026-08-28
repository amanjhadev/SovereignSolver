#ifndef SOVEREIGN_MILP_MILP_SOLVER_HPP
#define SOVEREIGN_MILP_MILP_SOLVER_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_types.hpp"
#include "sovereign/milp/bb_node.hpp"
#include "sovereign/milp/branching.hpp"

namespace sovereign::milp {

class MILPSolver {
private:
    MILPConfig config_;

public:
    MILPSolver() = default;
    explicit MILPSolver(const MILPConfig& config) : config_(config) {}

    MILPResult solve(const MILPModel& model) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_MILP_SOLVER_HPP
