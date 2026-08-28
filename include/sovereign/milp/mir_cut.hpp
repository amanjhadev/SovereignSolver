#ifndef SOVEREIGN_MILP_MIR_CUT_HPP
#define SOVEREIGN_MILP_MIR_CUT_HPP

#include "sovereign/milp/cut_types.hpp"
#include "sovereign/milp/milp_model.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include <vector>
#include <optional>

namespace sovereign::milp {

class CanonicalMIRCutGenerator {
private:
    Float64 tol_integrality_{1e-6};
    Float64 tol_violation_{1e-4};

public:
    CanonicalMIRCutGenerator() = default;
    explicit CanonicalMIRCutGenerator(Float64 tol_integrality, Float64 tol_violation = 1e-4);

    // Generates canonical Mixed-Integer Rounding (MIR) cuts when strict preconditions are met
    std::vector<Cut> generate_cuts(
        const MILPModel& model,
        const lp::LPModel& current_lp,
        const lp::SimplexState& state,
        Int64 origin_node_id = -1
    ) const;

    // Separates a single canonical MIR row if preconditions pass
    std::optional<Cut> separate_row(
        const MILPModel& model,
        std::span<const Index> row_cols,
        std::span<const Float64> row_vals,
        Float64 rhs,
        std::span<const Float64> current_sol,
        Int64 origin_node_id
    ) const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_MIR_CUT_HPP
