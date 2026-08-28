#include "sovereign/core/thread_pool.hpp"
#include <algorithm>

namespace sovereign::core {

ThreadPool::ThreadPool(std::size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    }

    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this, i](std::stop_token st) {
            tls_worker_id_ = i;
            while (!st.stop_requested()) {
                std::function<void(std::stop_token)> task;
                {
                    std::unique_lock lock(queue_mutex_);
                    cv_tasks_.wait(lock, st, [this]() {
                        return is_stopping_.load() || !task_queue_.empty();
                    });

                    if (is_stopping_.load() && task_queue_.empty()) {
                        return;
                    }

                    if (task_queue_.empty()) {
                        continue;
                    }

                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }

                if (task) {
                    task(st);
                }

                if (--active_tasks_ == 0) {
                    cv_done_.notify_all();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    request_stop();
}

void ThreadPool::enqueue(std::function<void(std::stop_token)> task) {
    {
        std::lock_guard lock(queue_mutex_);
        if (is_stopping_.load()) {
            return;
        }
        active_tasks_++;
        task_queue_.push(std::move(task));
    }
    cv_tasks_.notify_one();
}

void ThreadPool::wait_all() {
    std::unique_lock lock(queue_mutex_);
    cv_done_.wait(lock, [this]() {
        return active_tasks_.load() == 0 && task_queue_.empty();
    });
}

void ThreadPool::request_stop() {
    is_stopping_.store(true);
    for (auto& w : workers_) {
        w.request_stop();
    }
    cv_tasks_.notify_all();
    workers_.clear();
}

std::size_t ThreadPool::current_worker_id() {
    return tls_worker_id_;
}

} // namespace sovereign::core
