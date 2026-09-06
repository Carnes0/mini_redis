#include "mini_redis/commands/dispatcher.hpp"

#include <algorithm>
#include <cctype>
#include <span>

namespace mini_redis {
namespace {
std::string upper_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

ErrorString wrong_arguments(std::string_view command) {
    return {"ERR wrong number of arguments for '" + std::string(command) + "' command"};
}
} // namespace

CommandResult CommandDispatcher::execute(const CommandRequest& request) const {
    if (request.arguments.empty()) return ErrorString{"ERR empty command"};
    const auto command = upper_ascii(request.arguments.front());
    const auto size = request.arguments.size();

    if (command == "PING") {
        if (size == 1) return SimpleString{"PONG"};
        if (size == 2) return BulkString{request.arguments[1]};
        return wrong_arguments("ping");
    }
    if (command == "SET") {
        if (size != 3) return wrong_arguments("set");
        store_.set(request.arguments[1], request.arguments[2]);
        return SimpleString{"OK"};
    }
    if (command == "GET") {
        if (size != 2) return wrong_arguments("get");
        auto value = store_.get(request.arguments[1]);
        return value ? CommandResult{BulkString{std::move(*value)}} : CommandResult{NullBulkString{}};
    }
    if (command == "DEL" || command == "EXISTS") {
        if (size < 2) return wrong_arguments(command == "DEL" ? "del" : "exists");
        const std::span<const std::string> keys(request.arguments.data() + 1, size - 1);
        const auto count = command == "DEL" ? store_.erase(keys) : store_.count_existing(keys);
        return Integer{static_cast<std::int64_t>(count)};
    }
    return ErrorString{"ERR unknown command"};
}
} // namespace mini_redis
