#ifndef SOVEREIGN_MILP_COVER_CUT_HPP
#define SOVEREIGN_MILP_COVER_CUT_HPP

#include "sovereign/milp/cut_types.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include <vector>
#include <optional>

namespace sovereign::milp {

class MinimalCoverCutGenerator {
private:
    Float64 tol_violation_{1e-4};

public:
    MinimalCoverCutGenerator() = default;
    explicit MinimalCoverCutGenerator(Float64 tol_violation);

    // Generates unlifted minimal knapsack cover cuts
    std::vector<Cut> generate_cuts(
        const MILPModel& model,
        const lp::LPModel& current_lp,
        const lp::SimplexState& state,
        Int64 origin_node_id = -1
    ) const;

    std::optional<Cut> separate_knapsack_row(
        const MILPModel& model,
        std::span<const Index> row_cols,
        std::span<const Float64> row_vals,
        Float64 rhs,
        std::span<const Float64> current_sol,
        Int64 origin_node_id
    ) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_COVER_CUT_HPP
