#ifndef SOVEREIGN_MILP_WORK_STEALING_QUEUE_HPP
#define SOVEREIGN_MILP_WORK_STEALING_QUEUE_HPP

#include "sovereign/milp/bb_node.hpp"
#include "sovereign/milp/milp_types.hpp"
#include "sovereign/core/types.hpp"
#include <mutex>
#include <vector>
#include <queue>
#include <memory>
#include <optional>

namespace sovereign::milp {

// Node Comparator for ThreadSafeNodeQueue
struct BBNodePointerCompare {
    NodeSelectionRule rule{NodeSelectionRule::BestBoundFirst};
    ObjectiveSense sense{ObjectiveSense::Minimize};

    bool operator()(const std::shared_ptr<BBNode>& a, const std::shared_ptr<BBNode>& b) const {
        if (!a || !b) return a != nullptr;

        if (rule == NodeSelectionRule::DepthFirst) {
            if (a->depth != b->depth) {
                return a->depth < b->depth; // Larger depth first
            }
        }

        // BestBoundFirst
        if (sense == ObjectiveSense::Minimize) {
            if (std::abs(a->lp_objective - b->lp_objective) > 1e-9) {
                return a->lp_objective > b->lp_objective; // Smaller lower bound first
            }
        } else {
            if (std::abs(a->lp_objective - b->lp_objective) > 1e-9) {
                return a->lp_objective < b->lp_objective; // Larger upper bound first
            }
        }

        return a->node_id > b->node_id; // Smaller node ID first for determinism
    }
};

class ThreadSafeNodeQueue {
public:
    explicit ThreadSafeNodeQueue(
        NodeSelectionRule rule = NodeSelectionRule::BestBoundFirst,
        ObjectiveSense sense = ObjectiveSense::Minimize
    ) : comp_{rule, sense}, queue_(comp_) {}

    void clear() {
        std::lock_guard lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

    void push(std::shared_ptr<BBNode> node) {
        if (!node) return;
        std::lock_guard lock(mutex_);
        queue_.push(std::move(node));
    }

    void push_batch(std::vector<std::shared_ptr<BBNode>> nodes) {
        std::lock_guard lock(mutex_);
        for (auto& node : nodes) {
            if (node) {
                queue_.push(std::move(node));
            }
        }
    }

    std::shared_ptr<BBNode> pop() {
        std::lock_guard lock(mutex_);
        if (queue_.empty()) {
            return nullptr;
        }
        auto node = queue_.top();
        queue_.pop();
        return node;
    }

    bool empty() const {
        std::lock_guard lock(mutex_);
        return queue_.empty();
    }

    std::size_t size() const {
        std::lock_guard lock(mutex_);
        return queue_.size();
    }

    // Retrieve best bound among open nodes
    std::optional<Float64> get_best_bound() const {
        std::lock_guard lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        return queue_.top()->lp_objective;
    }

private:
    BBNodePointerCompare comp_;
    mutable std::mutex mutex_;
    std::priority_queue<
        std::shared_ptr<BBNode>,
        std::vector<std::shared_ptr<BBNode>>,
        BBNodePointerCompare
    > queue_;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_WORK_STEALING_QUEUE_HPP
