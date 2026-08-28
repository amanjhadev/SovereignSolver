#ifndef SOVEREIGN_MILP_THREAD_LOCAL_STATE_HPP
#define SOVEREIGN_MILP_THREAD_LOCAL_STATE_HPP

#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/presolve/presolve_engine.hpp"
#include "sovereign/milp/gomory_cut.hpp"
#include "sovereign/milp/mir_cut.hpp"
#include "sovereign/milp/cover_cut.hpp"
#include "sovereign/milp/feasibility_pump.hpp"
#include "sovereign/milp/diving_heuristic.hpp"
#include "sovereign/milp/rins_heuristic.hpp"
#include <random>
#include <memory>

namespace sovereign::milp {

struct ThreadLocalSolverState {
    std::size_t thread_id{0};
    std::mt19937_64 rng;

    // Dedicated thread-private LP solver (with private LU factor workspace)
    lp::DualSimplexSolver dual_simplex;

    // Dedicated thread-private presolve engine workspace
    presolve::PresolveEngine presolver;

    // Cut generators
    GomoryCutGenerator gomory_gen;
    CanonicalMIRCutGenerator mir_gen;
    MinimalCoverCutGenerator cover_gen;

    // Heuristics
    FeasibilityPumpHeuristic feasibility_pump;
    DivingHeuristic diving_heuristic;
    RINSHeuristic rins_heuristic;

    // Performance counters per thread
    Int64 lp_solves{0};
    Int64 cuts_generated{0};
    Int64 cuts_accepted{0};
    Int64 cuts_rejected{0};
    Int64 heuristic_incumbents_found{0};

    explicit ThreadLocalSolverState(std::size_t tid, std::uint64_t seed, const presolve::PresolveConfig& presolve_cfg = {})
        : thread_id(tid), rng(seed + tid), presolver(presolve_cfg) {}
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_THREAD_LOCAL_STATE_HPP
