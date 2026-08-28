#ifndef SOVEREIGN_MILP_GOMORY_CUT_HPP
#define SOVEREIGN_MILP_GOMORY_CUT_HPP

#include "sovereign/milp/cut_types.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include <vector>

namespace sovereign::milp {

class GomoryCutGenerator {
private:
    Float64 tol_integrality_{1e-6};
    Float64 tol_violation_{1e-4};

public:
    GomoryCutGenerator() = default;
    explicit GomoryCutGenerator(Float64 tol_integrality, Float64 tol_violation = 1e-4);

    // Generates valid Gomory fractional cuts using the complete 4-stage pipeline
    std::vector<Cut> generate_cuts(
        const MILPModel& model,
        const lp::LPModel& current_lp,
        const lp::SimplexState& state,
        Int64 origin_node_id = -1
    ) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_GOMORY_CUT_HPP
