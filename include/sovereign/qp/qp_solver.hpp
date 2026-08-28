#ifndef SOVEREIGN_QP_QP_SOLVER_HPP
#define SOVEREIGN_QP_QP_SOLVER_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/qp/qp_model.hpp"
#include "sovereign/qp/qp_types.hpp"
#include "sovereign/barrier/sparse_cholesky.hpp"

namespace sovereign::qp {

class QPSolver {
private:
    QPConfig config_;

public:
    QPSolver() = default;
    explicit QPSolver(const QPConfig& config) : config_(config) {}

    // Solves Convex QPModel using Primal-Dual Mehrotra Interior-Point Method
    QPResult solve(const QPModel& model) const;
};

} // namespace sovereign::qp

#endif // SOVEREIGN_QP_QP_SOLVER_HPP
