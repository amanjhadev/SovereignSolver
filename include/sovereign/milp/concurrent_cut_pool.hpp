#ifndef SOVEREIGN_MILP_CONCURRENT_CUT_POOL_HPP
#define SOVEREIGN_MILP_CONCURRENT_CUT_POOL_HPP

#include "sovereign/milp/cut_pool.hpp"
#include "sovereign/milp/cut_types.hpp"
#include <shared_mutex>
#include <vector>
#include <algorithm>

namespace sovereign::milp {

class ConcurrentCutPool {
public:
    explicit ConcurrentCutPool(Float64 min_violation = 1e-4, Float64 max_parallelism = 0.90)
        : pool_(min_violation, max_parallelism) {}

    void clear() {
        std::unique_lock lock(mutex_);
        pool_.clear();
    }

    // Attempt to add a globally valid cut. Local cuts are rejected from global pool.
    bool add_cut(const Cut& cut, std::span<const Float64> current_lp_sol) {
        if (cut.scope != CutScope::Global) {
            return false;
        }

        std::unique_lock lock(mutex_);
        return pool_.add_cut(cut, current_lp_sol);
    }

    // Sort cuts deterministically by provenance and batch insert
    std::size_t add_cuts_deterministic(std::vector<Cut> candidate_cuts, std::span<const Float64> current_lp_sol) {
        // Sort candidate cuts by origin_node_id, cut_type, variable_indices
        std::sort(candidate_cuts.begin(), candidate_cuts.end(), [](const Cut& a, const Cut& b) {
            if (a.origin_node_id != b.origin_node_id) return a.origin_node_id < b.origin_node_id;
            if (a.type != b.type) return static_cast<int>(a.type) < static_cast<int>(b.type);
            if (a.col_indices.size() != b.col_indices.size()) return a.col_indices.size() < b.col_indices.size();
            for (std::size_t k = 0; k < a.col_indices.size(); ++k) {
                if (a.col_indices[k] != b.col_indices[k]) {
                    return a.col_indices[k] < b.col_indices[k];
                }
                if (std::abs(a.coefficients[k] - b.coefficients[k]) > 1e-9) {
                    return a.coefficients[k] < b.coefficients[k];
                }
            }
            return a.rhs < b.rhs;
        });

        std::unique_lock lock(mutex_);
        std::size_t accepted = 0;
        for (const auto& cut : candidate_cuts) {
            if (cut.scope == CutScope::Global) {
                if (pool_.add_cut(cut, current_lp_sol)) {
                    accepted++;
                }
            }
        }
        return accepted;
    }

    // Retrieve copy of active global cuts
    std::vector<Cut> get_active_cuts() const {
        std::shared_lock lock(mutex_);
        return pool_.cuts();
    }

    std::size_t size() const {
        std::shared_lock lock(mutex_);
        return pool_.size();
    }

private:
    mutable std::shared_mutex mutex_;
    CutPool pool_;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_CONCURRENT_CUT_POOL_HPP
