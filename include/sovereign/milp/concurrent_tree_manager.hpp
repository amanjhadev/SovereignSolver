#ifndef SOVEREIGN_MILP_CONCURRENT_TREE_MANAGER_HPP
#define SOVEREIGN_MILP_CONCURRENT_TREE_MANAGER_HPP

#include "sovereign/milp/bb_node.hpp"
#include "sovereign/milp/work_stealing_queue.hpp"
#include "sovereign/milp/atomic_incumbent.hpp"
#include "sovereign/milp/milp_types.hpp"
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace sovereign::milp {

class ConcurrentTreeManager {
public:
    ConcurrentTreeManager(
        const MILPConfig& config,
        ObjectiveSense sense
    );

    void reset();

    // Push initial root node
    void push_root_node(std::shared_ptr<BBNode> root);

    // Push child nodes
    void push_child_nodes(std::vector<std::shared_ptr<BBNode>> children);

    // Pop next node to process (returns nullptr if tree is empty or terminated)
    std::shared_ptr<BBNode> pop_next_node(std::size_t worker_id);

    // Wave Synchronization for Deterministic Mode: pop all nodes at current depth level
    std::vector<std::shared_ptr<BBNode>> pop_wave_nodes();

    // Notify that a node evaluation has finished
    void notify_node_completed();

    // Check if search should terminate
    bool is_terminated() const { return is_terminated_.load(std::memory_order_relaxed); }
    void signal_termination() {
        is_terminated_.store(true, std::memory_order_relaxed);
        cv_work_.notify_all();
    }

    // Query global dual bound
    Float64 get_global_bound() const;

    // Performance counters
    void add_lp_solves(Int64 count) { total_lp_solves_.fetch_add(count, std::memory_order_relaxed); }
    void add_nodes_explored(Int64 count) { total_nodes_explored_.fetch_add(count, std::memory_order_relaxed); }

    Int64 nodes_explored() const { return total_nodes_explored_.load(std::memory_order_relaxed); }
    Int64 lp_solves() const { return total_lp_solves_.load(std::memory_order_relaxed); }
    std::size_t open_nodes_count() const;

private:
    MILPConfig config_;
    ObjectiveSense sense_;

    ThreadSafeNodeQueue global_queue_;
    std::atomic<Int64> active_in_flight_{0};
    std::atomic<bool> is_terminated_{false};

    std::atomic<Int64> total_nodes_explored_{0};
    std::atomic<Int64> total_lp_solves_{0};

    mutable std::mutex wave_mutex_;
    std::condition_variable cv_work_;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_CONCURRENT_TREE_MANAGER_HPP
