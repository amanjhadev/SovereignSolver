#ifndef SOVEREIGN_CORE_THREAD_POOL_HPP
#define SOVEREIGN_CORE_THREAD_POOL_HPP

#include "sovereign/core/types.hpp"
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stop_token>

namespace sovereign::core {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t num_threads = 0);
    ~ThreadPool();

    // Disable copy
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Post a task to pool
    void enqueue(std::function<void(std::stop_token)> task);

    // Wait until all pending tasks are completed
    void wait_all();

    // Request stop on all threads
    void request_stop();

    // Get active thread count
    std::size_t size() const { return workers_.size(); }

    // Query worker index of current thread (returns size_t max if caller is not a pool worker)
    static std::size_t current_worker_id();

private:
    std::vector<std::jthread> workers_;
    std::queue<std::function<void(std::stop_token)>> task_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable_any cv_tasks_;
    std::condition_variable cv_done_;
    std::atomic<std::size_t> active_tasks_{0};
    std::atomic<bool> is_stopping_{false};

    static inline thread_local std::size_t tls_worker_id_{std::numeric_limits<std::size_t>::max()};
};

} // namespace sovereign::core

#endif // SOVEREIGN_CORE_THREAD_POOL_HPP
