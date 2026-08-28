#ifndef SOVEREIGN_MILP_PARALLEL_MILP_SOLVER_HPP
#define SOVEREIGN_MILP_PARALLEL_MILP_SOLVER_HPP

#include "sovereign/milp/milp_model.hpp"
#include "sovereign/milp/milp_types.hpp"
#include "sovereign/milp/atomic_incumbent.hpp"
#include "sovereign/milp/concurrent_tree_manager.hpp"
#include "sovereign/milp/concurrent_cut_pool.hpp"
#include "sovereign/milp/thread_local_state.hpp"
#include "sovereign/core/thread_pool.hpp"
#include <memory>
#include <vector>

namespace sovereign::milp {

class ParallelMILPSolver {
public:
    explicit ParallelMILPSolver(const MILPConfig& config = MILPConfig{});

    // Main parallel solve entry point
    MILPResult solve(const MILPModel& model);

private:
    MILPConfig config_;

    // Private helper for Deterministic Mode (Wavefront Level Synchronization)
    MILPResult solve_deterministic(const MILPModel& model);

    // Private helper for MaxThroughput Mode (Asynchronous Concurrent Solving)
    MILPResult solve_max_throughput(const MILPModel& model);
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_PARALLEL_MILP_SOLVER_HPP
