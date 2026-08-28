#ifndef SOVEREIGN_QP_PRESOLVE_QP_SOLVER_HPP
#define SOVEREIGN_QP_PRESOLVE_QP_SOLVER_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/config.hpp"
#include "sovereign/qp/qp_model.hpp"
#include "sovereign/qp/qp_solver.hpp"
#include "sovereign/presolve/presolve_engine.hpp"
#include "sovereign/presolve/postsolve_engine.hpp"

namespace sovereign::qp {

class PresolveQPSolver {
private:
    SolverConfig config_;
    presolve::PresolveConfig presolve_config_;
    QPConfig qp_config_;

public:
    PresolveQPSolver();
    explicit PresolveQPSolver(
        const SolverConfig& config,
        const presolve::PresolveConfig& presolve_config = presolve::PresolveConfig{},
        const QPConfig& qp_config = QPConfig{}
    );

    // Solves QPModel using Presolve -> QP Engine -> Postsolve
    // Converts QPResult to standard SimplexResult at boundary
    lp::SimplexResult solve(const QPModel& orig_qp_model);
};

} // namespace sovereign::qp

#endif // SOVEREIGN_QP_PRESOLVE_QP_SOLVER_HPP
