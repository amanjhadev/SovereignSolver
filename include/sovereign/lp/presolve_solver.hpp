#ifndef SOVEREIGN_LP_PRESOLVE_SOLVER_HPP
#define SOVEREIGN_LP_PRESOLVE_SOLVER_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/simplex_types.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/presolve/presolve_engine.hpp"
#include "sovereign/presolve/postsolve_engine.hpp"

namespace sovereign::lp {

class PresolveSolver {
private:
    SolverConfig config_;
    presolve::PresolveConfig presolve_config_;

public:
    PresolveSolver() = default;
    explicit PresolveSolver(const SolverConfig& config, const presolve::PresolveConfig& presolve_config = presolve::PresolveConfig{});

    // Solves model using Presolve -> Dual Simplex Engine -> Postsolve
    SimplexResult solve(const LPModel& orig_model);
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_PRESOLVE_SOLVER_HPP
