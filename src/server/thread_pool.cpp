#include "mini_redis/server/thread_pool.hpp"

#include <iostream>
#include <stdexcept>

namespace mini_redis {
ThreadPool::ThreadPool(std::size_t worker_count) {
    if (worker_count == 0) throw std::invalid_argument("worker count must be positive");
    workers_.reserve(worker_count);
    for (std::size_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    ready_.notify_all();
    for (auto& worker : workers_) if (worker.joinable()) worker.join();
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        if (stopping_) throw std::runtime_error("thread pool is stopping");
        tasks_.push(std::move(task));
    }
    ready_.notify_one();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            ready_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        try {
            task();
        } catch (const std::exception& error) {
            std::cerr << "worker task failed: " << error.what() << '\n';
        } catch (...) {
            std::cerr << "worker task failed: unknown exception\n";
        }
    }
}
} // namespace mini_redis
