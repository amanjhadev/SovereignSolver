#include "sovereign/milp/atomic_incumbent.hpp"
#include <cmath>

namespace sovereign::milp {

void AtomicIncumbent::reset() {
    std::lock_guard lock(solution_mutex_);
    has_incumbent_.store(false, std::memory_order_relaxed);
    best_objective_.store(0.0, std::memory_order_relaxed);
    version_.store(0, std::memory_order_relaxed);
    best_solution_.clear();
}

bool AtomicIncumbent::update_incumbent(
    const std::vector<Float64>& candidate_solution,
    Float64 candidate_obj,
    ObjectiveSense sense,
    Float64 tol_gap
) {
    if (std::isnan(candidate_obj) || std::isinf(candidate_obj)) {
        return false;
    }

    std::lock_guard lock(solution_mutex_);

    bool is_better = false;
    if (!has_incumbent_.load(std::memory_order_relaxed)) {
        is_better = true;
    } else {
        Float64 current_best = best_objective_.load(std::memory_order_relaxed);
        if (sense == ObjectiveSense::Minimize) {
            if (candidate_obj < current_best - tol_gap) {
                is_better = true;
            }
        } else { // Maximize
            if (candidate_obj > current_best + tol_gap) {
                is_better = true;
            }
        }
    }

    if (is_better) {
        best_solution_ = candidate_solution;
        best_objective_.store(candidate_obj, std::memory_order_release);
        has_incumbent_.store(true, std::memory_order_release);
        version_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    return false;
}

std::vector<Float64> AtomicIncumbent::get_solution() const {
    std::lock_guard lock(solution_mutex_);
    return best_solution_;
}

bool AtomicIncumbent::can_fathom_node(Float64 node_bound, ObjectiveSense sense, Float64 tol_gap) const {
    if (!has_incumbent_.load(std::memory_order_relaxed)) {
        return false;
    }

    Float64 z_inc = best_objective_.load(std::memory_order_relaxed);
    if (sense == ObjectiveSense::Minimize) {
        return (node_bound >= z_inc - tol_gap);
    } else { // Maximize
        return (node_bound <= z_inc + tol_gap);
    }
}

} // namespace sovereign::milp
