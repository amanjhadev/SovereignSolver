#ifndef SOVEREIGN_LP_PRIMAL_SIMPLEX_HPP
#define SOVEREIGN_LP_PRIMAL_SIMPLEX_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/lp/simplex_types.hpp"
#include "sovereign/lp/simplex_state.hpp"

namespace sovereign::lp {

class PrimalSimplexSolver {
private:
    SolverConfig config_;
    Float64 tol_primal_{1e-6};
    Float64 tol_dual_{1e-7};
    Float64 tol_pivot_{1e-9};
    Int64 max_iterations_{10000};

public:
    PrimalSimplexSolver() = default;
    explicit PrimalSimplexSolver(const SolverConfig& config);

    // Solves primal feasible LP model using Primal Revised Simplex (used for Phase I)
    SimplexStatus solve(SimplexState& state, Int64& total_iters);
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_PRIMAL_SIMPLEX_HPP
