#ifndef SOVEREIGN_MILP_MILP_TYPES_HPP
#define SOVEREIGN_MILP_MILP_TYPES_HPP

#include "sovereign/core/types.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace sovereign::milp {

enum class MILPStatus : std::uint8_t {
    Optimal          = 0, // Global optimal integer solution proven (MIP gap <= tol_mip_gap)
    Infeasible       = 1, // Integer infeasibility proven (queue empty, no incumbent)
    Unbounded        = 2, // Integer-feasible unbounded ray established
    Unresolved       = 3, // Continuous LP relaxation unbounded but no integer ray established
    NodeLimit        = 4, // Maximum node limit reached (incumbent returned if available)
    TimeLimit        = 5, // Maximum execution time reached (incumbent returned if available)
    NumericalFailure = 6  // LP solver numerical failure
};

enum class BranchingRule : std::uint8_t {
    MostFractional   = 0, // Selects variable with fractionality closest to 0.5
    FirstFractional  = 1  // Selects first fractional variable by index
};

enum class NodeSelectionRule : std::uint8_t {
    BestBoundFirst   = 0, // Selects node with best LP relaxation bound
    DepthFirst       = 1  // Selects deepest node (largest tree depth)
};

enum class ParallelMode : std::uint8_t {
    Serial          = 0, // Single-threaded execution
    Deterministic   = 1, // Multi-threaded wave execution with deterministic node/cut ordering
    MaxThroughput   = 2  // Multi-threaded asynchronous work-stealing execution
};

struct MILPConfig {
    Float64 tol_integrality{1e-6};  // Max deviation from integer: |x - round(x)|
    Float64 tol_mip_gap{1e-4};      // Relative MIP gap tolerance
    Float64 tol_primal{1e-6};       // LP primal feasibility tolerance
    Float64 tol_dual{1e-6};         // LP dual feasibility tolerance
    Int64 max_nodes{100000};        // Maximum B&B nodes to process
    Float64 max_time_sec{3600.0};   // Maximum runtime in seconds
    BranchingRule branching_rule{BranchingRule::MostFractional};
    NodeSelectionRule node_selection{NodeSelectionRule::BestBoundFirst};

    // Batch 8 Cutting-Plane & Heuristic Configuration
    bool enable_cuts{true};
    Int32 max_cut_passes{5};
    bool enable_feasibility_pump{true};
    bool enable_diving{true};
    bool enable_rins{false}; // Experimental, default disabled

    // Batch 9 Parallel Engine Configuration
    Int64 num_threads{0}; // 0 = auto-detect hardware concurrency (serial if 1)
    ParallelMode parallel_mode{ParallelMode::Deterministic};
    std::uint64_t random_seed{42};
    bool enable_work_stealing{true};
};

struct MILPResult {
    MILPStatus status{MILPStatus::Infeasible};
    Float64 objective_value{0.0};
    std::vector<Float64> primal_solution;
    Float64 best_bound{0.0};
    Float64 mip_gap{1.0};
    Int64 nodes_explored{0};
    Int64 lp_solves{0};
    Int64 open_nodes_remaining{0};
    double solve_time_ms{0.0};

    // Batch 8 Performance Counters
    Int64 cuts_generated{0};
    Int64 cuts_accepted{0};
    Int64 cuts_rejected{0};
    Int64 heuristic_incumbents_found{0};

    std::string message;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_MILP_TYPES_HPP
