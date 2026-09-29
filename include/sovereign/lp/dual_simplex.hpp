#ifndef SOVEREIGN_LP_DUAL_SIMPLEX_HPP
#define SOVEREIGN_LP_DUAL_SIMPLEX_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/simplex_types.hpp"
#include "sovereign/lp/simplex_state.hpp"
#include "sovereign/lp/ratio_test.hpp"
#include "sovereign/lp/devex_pricing.hpp"

namespace sovereign::lp {

class DualSimplexSolver {
private:
    SolverConfig config_;

    Float64 tol_primal_{1e-6};
    Float64 tol_dual_{1e-7};
    Float64 tol_pivot_{1e-9};
    Int64 max_iterations_{10000};

public:
    DualSimplexSolver() = default;
    explicit DualSimplexSolver(const SolverConfig& config);

    // Solves general LP model using Dual Revised Simplex with Phase I & Phase II
    SimplexResult solve(const LPModel& model);
    SimplexResult solve(const LPModel& model, const Basis& initial_basis);

private:
    // Internal Phase I auxiliary problem solver using Primal Simplex
    SimplexStatus solve_phase_1(const LPModel& std_model, LPModel& phase2_model, SimplexState& state, Int64& total_iters);

    // Internal Phase II optimization solver using Dual Simplex
    SimplexStatus solve_phase_2(SimplexState& state, Int64& total_iters);

    // Solution Recovery & Original Feasibility Checks
    SimplexResult recover_solution(
        const LPModel& orig_model,
        const LPModel& std_model,
        const SimplexState& state,
        SimplexStatus status,
        Int64 total_iters,
        const std::string& msg
    ) const;
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_DUAL_SIMPLEX_HPP
