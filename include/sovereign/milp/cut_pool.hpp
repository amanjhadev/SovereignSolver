#ifndef SOVEREIGN_MILP_CUT_POOL_HPP
#define SOVEREIGN_MILP_CUT_POOL_HPP

#include "sovereign/milp/cut_types.hpp"
#include <span>
#include <vector>
#include <set>

namespace sovereign::milp {

struct CutValidationResult {
    bool is_valid{false};
    std::string rejection_reason;
};

class CutPool {
private:
    std::vector<Cut> cuts_;

    Float64 min_violation_{1e-4};
    Float64 max_parallelism_{0.90};
    Float64 max_coeff_ratio_{1e6};

public:
    CutPool() = default;
    explicit CutPool(Float64 min_violation, Float64 max_parallelism = 0.90);

    // 4-Stage Validity Verification
    CutValidationResult validate_cut(const Cut& cut, std::span<const Float64> current_sol) const;

    // Cut Addition with Validity & Performance Filtering
    bool add_cut(Cut cut, std::span<const Float64> current_sol);

    // Query & Filtering
    std::vector<Cut> get_valid_cuts_for_node(
        Int64 target_node_id,
        const std::set<Int64>& node_ancestors,
        std::size_t max_cuts = 50
    ) const;

    void purge_inactive_cuts(std::span<const Float64> slack_values, Int64 max_slack_age = 5);
    void clear() noexcept { cuts_.clear(); }

    std::size_t size() const noexcept { return cuts_.size(); }
    const std::vector<Cut>& cuts() const noexcept { return cuts_; }
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_CUT_POOL_HPP
