#include "mini_redis/commands/dispatcher.hpp"
#include "mini_redis/server/tcp_server.hpp"
#include "mini_redis/storage/store.hpp"

#include <charconv>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
std::atomic<HANDLE> stop_event{nullptr};

BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT) {
        if (const HANDLE event = stop_event.load()) SetEvent(event);
        return TRUE;
    }
    return FALSE;
}

bool parse_port(std::string_view text, std::uint16_t& port) {
    unsigned value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || value == 0 || value > 65535) {
        return false;
    }
    port = static_cast<std::uint16_t>(value);
    return true;
}
} // namespace

int main(int argc, char** argv) {
    std::uint16_t port = 6380;
    if (argc == 3 && std::string_view(argv[1]) == "--port") {
        if (!parse_port(argv[2], port)) {
            std::cerr << "invalid port: " << argv[2] << '\n';
            return 2;
        }
    } else if (argc != 1) {
        std::cerr << "usage: mini_redis [--port 1-65535]\n";
        return 2;
    }

    mini_redis::KeyValueStore store;
    mini_redis::CommandDispatcher dispatcher(store);
    mini_redis::TcpServer server(port, 4, dispatcher);
    const HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (event == nullptr) {
        std::cerr << "failed to create stop event\n";
        return 1;
    }
    stop_event.store(event);
    SetConsoleCtrlHandler(console_handler, TRUE);
    std::thread stop_monitor([&server, event] {
        WaitForSingleObject(event, INFINITE);
        server.stop();
    });
    const int result = server.run();
    SetConsoleCtrlHandler(console_handler, FALSE);
    SetEvent(event);
    stop_monitor.join();
    stop_event.store(nullptr);
    CloseHandle(event);
    return result;
}
