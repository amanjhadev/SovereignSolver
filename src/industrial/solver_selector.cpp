#include "sovereign/industrial/solver_selector.hpp"

namespace sovereign::industrial {

SelectionResult SolverSelector::select(const lp::LPModel& model, bool force_barrier) {
    std::size_t nz = model.get_csc_matrix().num_nonzeros();
    std::size_t m = model.num_constraints();
    std::size_t n = model.num_variables();

    if (force_barrier) {
        return {SelectedEngine::BarrierLP, "Rule: Barrier solver explicitly requested by configuration"};
    }

    if (nz > 50000 || (m > 1000 && n > 1000 && static_cast<double>(nz) / (m * n) > 0.10)) {
        return {SelectedEngine::BarrierLP, "Rule: High dimension / dense matrix threshold exceeded (NZ > 50k or Density > 10%)"};
    }

    return {SelectedEngine::DualSimplex, "Rule: Continuous LP default dual simplex selection"};
}

SelectionResult SolverSelector::select(const qp::QPModel& /*model*/) {
    return {SelectedEngine::BarrierQP, "Rule: Quadratic objective terms present -> Interior Point QP solver"};
}

SelectionResult SolverSelector::select(const milp::MILPModel& /*model*/) {
    return {SelectedEngine::ParallelMILP, "Rule: Discrete integer/binary variables present -> Parallel Branch-and-Bound solver"};
}

} // namespace sovereign::industrial
