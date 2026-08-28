#ifndef SOVEREIGN_PRESOLVE_POSTSOLVE_ENGINE_HPP
#define SOVEREIGN_PRESOLVE_POSTSOLVE_ENGINE_HPP

#include "sovereign/presolve/presolve_types.hpp"
#include "sovereign/presolve/presolve_stack.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/simplex_types.hpp"

namespace sovereign::presolve {

class PostsolveEngine {
private:
    Float64 tol_primal_{1e-6};

public:
    PostsolveEngine() = default;
    explicit PostsolveEngine(Float64 tol_primal) : tol_primal_(tol_primal) {}

    // Reconstructs original model primal solution, verifies bounds and residuals
    lp::SimplexResult postsolve(
        const lp::LPModel& orig_model,
        const lp::LPModel& presolved_model,
        const lp::SimplexResult& presolved_result,
        const PresolveStack& stack
    ) const;
};

} // namespace sovereign::presolve

#endif // SOVEREIGN_PRESOLVE_POSTSOLVE_ENGINE_HPP
