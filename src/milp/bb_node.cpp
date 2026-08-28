#include "sovereign/milp/bb_node.hpp"
#include <algorithm>
#include <cmath>

namespace sovereign::milp {

void BBTreeManager::push_node(BBNode node) {
    open_nodes_.push_back(std::move(node));
}

BBNode BBTreeManager::pop_node(NodeSelectionRule rule, ObjectiveSense sense) {
    if (open_nodes_.empty()) {
        return BBNode{};
    }

    std::size_t best_idx = 0;

    if (rule == NodeSelectionRule::DepthFirst) {
        // Find deepest node
        Int32 max_depth = open_nodes_[0].depth;
        for (std::size_t i = 1; i < open_nodes_.size(); ++i) {
            if (open_nodes_[i].depth > max_depth) {
                max_depth = open_nodes_[i].depth;
                best_idx = i;
            }
        }
    } else { // BestBoundFirst
        Float64 best_val = open_nodes_[0].lp_objective;
        for (std::size_t i = 1; i < open_nodes_.size(); ++i) {
            Float64 val = open_nodes_[i].lp_objective;
            if (sense == ObjectiveSense::Minimize) {
                if (val < best_val) {
                    best_val = val;
                    best_idx = i;
                }
            } else {
                if (val > best_val) {
                    best_val = val;
                    best_idx = i;
                }
            }
        }
    }

    BBNode chosen = std::move(open_nodes_[best_idx]);
    open_nodes_.erase(open_nodes_.begin() + best_idx);
    return chosen;
}

Float64 BBTreeManager::best_open_bound(ObjectiveSense sense) const {
    if (open_nodes_.empty()) {
        return (sense == ObjectiveSense::Minimize) ? INFINITY_VAL : -INFINITY_VAL;
    }

    Float64 best_val = open_nodes_[0].lp_objective;
    for (std::size_t i = 1; i < open_nodes_.size(); ++i) {
        Float64 val = open_nodes_[i].lp_objective;
        if (sense == ObjectiveSense::Minimize) {
            if (val < best_val) best_val = val;
        } else {
            if (val > best_val) best_val = val;
        }
    }
    return best_val;
}

void BBTreeManager::prune_nodes_by_bound(Float64 incumbent_obj, Float64 tol_gap, ObjectiveSense sense) {
    auto it = open_nodes_.begin();
    while (it != open_nodes_.end()) {
        bool prune = false;
        if (sense == ObjectiveSense::Minimize) {
            if (it->lp_objective > incumbent_obj + tol_gap) {
                prune = true;
            }
        } else { // Maximize
            if (it->lp_objective < incumbent_obj - tol_gap) {
                prune = true;
            }
        }

        if (prune) {
            it = open_nodes_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace sovereign::milp
