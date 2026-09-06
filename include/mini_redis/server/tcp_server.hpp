#pragma once

#include "mini_redis/commands/dispatcher.hpp"
#include "mini_redis/server/thread_pool.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace mini_redis {
class TcpServer {
public:
    TcpServer(std::uint16_t port, std::size_t workers, CommandDispatcher& dispatcher);
    ~TcpServer();
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    int run();
    void stop();

private:
    void serve_client(std::uintptr_t socket_value);
    void close_client(std::uintptr_t socket_value);

    std::uint16_t port_;
    CommandDispatcher& dispatcher_;
    std::unique_ptr<ThreadPool> pool_;
    const std::size_t max_clients_;
    std::atomic<bool> stopping_{false};
    std::atomic<std::uintptr_t> listener_{};
    std::mutex clients_mutex_;
    std::unordered_set<std::uintptr_t> clients_;
    bool winsock_started_{false};
};
} // namespace mini_redis
