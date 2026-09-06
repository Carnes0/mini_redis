#include "mini_redis/server/tcp_server.hpp"

#include "mini_redis/protocol/resp.hpp"

#ifndef _WIN32
#error "The MVP network adapter currently targets Windows WinSock2."
#endif

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mini_redis {
namespace {
bool send_all(SOCKET socket, std::string_view bytes) {
    std::size_t sent = 0;
    while (sent < bytes.size()) {
        const auto remaining = bytes.size() - sent;
        const auto chunk = remaining > static_cast<std::size_t>(INT_MAX)
            ? INT_MAX : static_cast<int>(remaining);
        const int result = send(socket, bytes.data() + sent, chunk, 0);
        if (result == SOCKET_ERROR || result == 0) return false;
        sent += static_cast<std::size_t>(result);
    }
    return true;
}
} // namespace

TcpServer::TcpServer(std::uint16_t port, std::size_t workers, CommandDispatcher& dispatcher)
    : port_(port), dispatcher_(dispatcher), pool_(std::make_unique<ThreadPool>(workers)),
      max_clients_(workers) {}

TcpServer::~TcpServer() {
    stop();
    pool_.reset();
    if (winsock_started_) WSACleanup();
}

int TcpServer::run() {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
    winsock_started_ = true;

    const SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        std::cerr << "socket creation failed: " << WSAGetLastError() << '\n';
        return 1;
    }
    listener_.store(static_cast<std::uintptr_t>(listener));
    if (stopping_.load()) {
        const auto owned = listener_.exchange(0);
        if (owned == static_cast<std::uintptr_t>(listener)) closesocket(listener);
        return 0;
    }

    BOOL reuse = TRUE;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port_);
    if (bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
        listen(listener, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "bind/listen failed: " << WSAGetLastError() << '\n';
        const auto owned = listener_.exchange(0);
        if (owned == static_cast<std::uintptr_t>(listener)) closesocket(listener);
        return 1;
    }

    std::cout << "mini_redis listening on 127.0.0.1:" << port_ << '\n';
    while (!stopping_.load()) {
        const SOCKET client = accept(listener, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            if (!stopping_.load()) std::cerr << "accept failed: " << WSAGetLastError() << '\n';
            break;
        }
        {
            std::lock_guard lock(clients_mutex_);
            if (clients_.size() >= max_clients_) {
                send_all(client, encode(ErrorString{"BUSY maximum client count reached"}));
                closesocket(client);
                continue;
            }
            clients_.insert(static_cast<std::uintptr_t>(client));
        }
        try {
            pool_->submit([this, client] { serve_client(static_cast<std::uintptr_t>(client)); });
        } catch (...) {
            close_client(static_cast<std::uintptr_t>(client));
            throw;
        }
    }
    return 0;
}

void TcpServer::stop() {
    stopping_.store(true);
    const auto listener = listener_.exchange(0);
    if (listener != 0) closesocket(static_cast<SOCKET>(listener));
    std::lock_guard lock(clients_mutex_);
    for (const auto client : clients_) {
        shutdown(static_cast<SOCKET>(client), SD_BOTH);
    }
}

void TcpServer::serve_client(std::uintptr_t socket_value) {
    const SOCKET client = static_cast<SOCKET>(socket_value);
    RespParser parser;
    std::string buffer;
    std::array<char, 8192> chunk{};

    while (!stopping_.load()) {
        const int received = recv(client, chunk.data(), static_cast<int>(chunk.size()), 0);
        if (received <= 0) break;
        buffer.append(chunk.data(), static_cast<std::size_t>(received));
        while (!buffer.empty()) {
            auto parsed = parser.parse(buffer);
            if (parsed.status == ParseStatus::incomplete) {
                if (buffer.size() > max_request_size) {
                    send_all(client, encode(ErrorString{"ERR request exceeds 1 MiB"}));
                    close_client(socket_value);
                    return;
                }
                break;
            }
            if (parsed.status == ParseStatus::protocol_error) {
                send_all(client, encode(ErrorString{"ERR Protocol error: " + parsed.error}));
                close_client(socket_value);
                return;
            }
            const auto response = encode(dispatcher_.execute(parsed.request));
            if (!send_all(client, response)) {
                close_client(socket_value);
                return;
            }
            buffer.erase(0, parsed.consumed);
        }
    }
    close_client(socket_value);
}

void TcpServer::close_client(std::uintptr_t socket_value) {
    {
        std::lock_guard lock(clients_mutex_);
        clients_.erase(socket_value);
    }
    closesocket(static_cast<SOCKET>(socket_value));
}
} // namespace mini_redis
