#include "sovereign/milp/concurrent_tree_manager.hpp"

namespace sovereign::milp {

ConcurrentTreeManager::ConcurrentTreeManager(
    const MILPConfig& config,
    ObjectiveSense sense
) : config_(config), sense_(sense), global_queue_(config.node_selection, sense) {}

void ConcurrentTreeManager::reset() {
    global_queue_.clear();
    active_in_flight_.store(0, std::memory_order_relaxed);
    is_terminated_.store(false, std::memory_order_relaxed);
    total_nodes_explored_.store(0, std::memory_order_relaxed);
    total_lp_solves_.store(0, std::memory_order_relaxed);
}

void ConcurrentTreeManager::push_root_node(std::shared_ptr<BBNode> root) {
    if (!root) return;
    global_queue_.push(std::move(root));
    cv_work_.notify_all();
}

void ConcurrentTreeManager::push_child_nodes(std::vector<std::shared_ptr<BBNode>> children) {
    if (children.empty()) return;
    global_queue_.push_batch(std::move(children));
    cv_work_.notify_all();
}

std::shared_ptr<BBNode> ConcurrentTreeManager::pop_next_node(std::size_t worker_id) {
    (void)worker_id;
    while (!is_terminated_.load(std::memory_order_relaxed)) {
        auto node = global_queue_.pop();
        if (node) {
            active_in_flight_.fetch_add(1, std::memory_order_relaxed);
            return node;
        }

        if (active_in_flight_.load(std::memory_order_relaxed) == 0 && global_queue_.empty()) {
            // All nodes explored, tree empty
            signal_termination();
            return nullptr;
        }

        // Wait for new work or termination
        std::unique_lock lock(wave_mutex_);
        cv_work_.wait_for(lock, std::chrono::milliseconds(5), [this]() {
            return is_terminated_.load(std::memory_order_relaxed) ||
                   !global_queue_.empty() ||
                   active_in_flight_.load(std::memory_order_relaxed) == 0;
        });

        if (global_queue_.empty() && active_in_flight_.load(std::memory_order_relaxed) == 0) {
            signal_termination();
            return nullptr;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<BBNode>> ConcurrentTreeManager::pop_wave_nodes() {
    std::vector<std::shared_ptr<BBNode>> wave;
    while (!global_queue_.empty()) {
        auto node = global_queue_.pop();
        if (node) {
            wave.push_back(std::move(node));
        }
    }
    return wave;
}

void ConcurrentTreeManager::notify_node_completed() {
    active_in_flight_.fetch_sub(1, std::memory_order_relaxed);
    cv_work_.notify_all();
}

Float64 ConcurrentTreeManager::get_global_bound() const {
    auto opt_bound = global_queue_.get_best_bound();
    if (opt_bound.has_value()) {
        return opt_bound.value();
    }
    return (sense_ == ObjectiveSense::Minimize) ? -INFINITY_VAL : INFINITY_VAL;
}

std::size_t ConcurrentTreeManager::open_nodes_count() const {
    return global_queue_.size();
}

} // namespace sovereign::milp
