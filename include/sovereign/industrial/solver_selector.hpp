#ifndef SOVEREIGN_INDUSTRIAL_SOLVER_SELECTOR_HPP
#define SOVEREIGN_INDUSTRIAL_SOLVER_SELECTOR_HPP

#include "sovereign/lp/lp_model.hpp"
#include "sovereign/qp/qp_model.hpp"
#include "sovereign/milp/milp_model.hpp"
#include <string>

namespace sovereign::industrial {

enum class SelectedEngine {
    DualSimplex,
    BarrierLP,
    BarrierQP,
    ParallelMILP
};

struct SelectionResult {
    SelectedEngine engine;
    std::string rationale;
};

class SolverSelector {
public:
    // Select solver engine for continuous LP model
    static SelectionResult select(const lp::LPModel& model, bool force_barrier = false);

    // Select solver engine for QP model
    static SelectionResult select(const qp::QPModel& model);

    // Select solver engine for MILP model
    static SelectionResult select(const milp::MILPModel& model);
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_SOLVER_SELECTOR_HPP
