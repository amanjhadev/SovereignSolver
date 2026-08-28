#ifndef SOVEREIGN_MILP_ATOMIC_INCUMBENT_HPP
#define SOVEREIGN_MILP_ATOMIC_INCUMBENT_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/simplex_types.hpp"
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>

namespace sovereign::milp {

class AtomicIncumbent {
public:
    AtomicIncumbent() = default;

    // Reset incumbent state
    void reset();

    // Safely attempt to update incumbent with candidate solution.
    // Returns true if new solution was accepted as strictly superior.
    bool update_incumbent(
        const std::vector<Float64>& candidate_solution,
        Float64 candidate_obj,
        ObjectiveSense sense,
        Float64 tol_gap = 1e-6
    );

    bool has_incumbent() const { return has_incumbent_.load(std::memory_order_relaxed); }
    Float64 best_objective() const { return best_objective_.load(std::memory_order_relaxed); }
    std::uint64_t version() const { return version_.load(std::memory_order_relaxed); }

    // Retrieve copy of current best primal solution vector
    std::vector<Float64> get_solution() const;

    // Check whether a node bound can be fathomed by the current incumbent
    bool can_fathom_node(Float64 node_bound, ObjectiveSense sense, Float64 tol_gap = 1e-4) const;

private:
    std::atomic<bool> has_incumbent_{false};
    std::atomic<Float64> best_objective_{0.0};
    std::atomic<std::uint64_t> version_{0};

    mutable std::mutex solution_mutex_;
    std::vector<Float64> best_solution_;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_ATOMIC_INCUMBENT_HPP
