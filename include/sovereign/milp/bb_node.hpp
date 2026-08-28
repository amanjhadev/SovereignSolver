#ifndef SOVEREIGN_MILP_BB_NODE_HPP
#define SOVEREIGN_MILP_BB_NODE_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/basis.hpp"
#include "sovereign/milp/milp_types.hpp"
#include <vector>
#include <string>

namespace sovereign::milp {

struct VariableBoundChange {
    Index col_index{-1};
    Float64 new_lower_bound{-INFINITY_VAL};
    Float64 new_upper_bound{INFINITY_VAL};
};

struct BranchingLinearConstraint {
    std::string name;
    lp::ConstraintSense sense{lp::ConstraintSense::LessEqual};
    Float64 rhs{0.0};
    std::vector<Index> col_indices;
    std::vector<Float64> coefficients;
};

struct BBNode {
    Int64 node_id{-1};
    Int64 parent_id{-1};
    Int32 depth{0};
    Float64 lp_objective{0.0};

    // Dual branch representation
    std::vector<VariableBoundChange> bound_changes;
    std::vector<BranchingLinearConstraint> branching_linear_constraints;

    lp::Basis warm_start_basis;
};

class BBTreeManager {
private:
    std::vector<BBNode> open_nodes_;
    Int64 next_node_id_{0};

public:
    BBTreeManager() = default;

    Int64 create_node_id() noexcept { return next_node_id_++; }

    void push_node(BBNode node);
    BBNode pop_node(NodeSelectionRule rule, ObjectiveSense sense);

    bool empty() const noexcept { return open_nodes_.empty(); }
    std::size_t open_node_count() const noexcept { return open_nodes_.size(); }

    Float64 best_open_bound(ObjectiveSense sense) const;
    void prune_nodes_by_bound(Float64 incumbent_obj, Float64 tol_gap, ObjectiveSense sense);
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_BB_NODE_HPP
