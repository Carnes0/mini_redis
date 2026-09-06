#include "mini_redis/protocol/resp.hpp"

#include <charconv>
#include <limits>
#include <type_traits>

namespace mini_redis {
namespace {
enum class LineStatus { complete, incomplete, invalid };

LineStatus read_number_line(std::string_view bytes, std::size_t start, std::int64_t& value,
                            std::size_t& next) {
    const auto end = bytes.find("\r\n", start);
    if (end == std::string_view::npos) return LineStatus::incomplete;
    if (end == start) return LineStatus::invalid;
    const char* first = bytes.data() + start;
    const char* last = bytes.data() + end;
    const auto parsed = std::from_chars(first, last, value);
    if (parsed.ec != std::errc{} || parsed.ptr != last) return LineStatus::invalid;
    next = end + 2;
    return LineStatus::complete;
}

ParseResult protocol_error(std::string message) {
    return {ParseStatus::protocol_error, {}, 0, std::move(message)};
}
} // namespace

ParseResult RespParser::parse(std::string_view bytes) const {
    if (bytes.empty()) return {};
    if (bytes.front() != '*') return protocol_error("expected array prefix");

    std::int64_t count = 0;
    std::size_t position = 0;
    const auto array_line = read_number_line(bytes, 1, count, position);
    if (array_line == LineStatus::incomplete) return {};
    if (array_line == LineStatus::invalid || count <= 0 || count > 1024) {
        return protocol_error("invalid array length");
    }

    CommandRequest request;
    request.arguments.reserve(static_cast<std::size_t>(count));
    for (std::int64_t i = 0; i < count; ++i) {
        if (position > max_request_size) return protocol_error("request exceeds 1 MiB");
        if (position >= bytes.size()) return {};
        if (bytes[position] != '$') return protocol_error("expected bulk string");
        std::int64_t length = 0;
        std::size_t payload_start = 0;
        const auto bulk_line = read_number_line(bytes, position + 1, length, payload_start);
        if (bulk_line == LineStatus::incomplete) return {};
        if (bulk_line == LineStatus::invalid || length < 0 ||
            length > static_cast<std::int64_t>(max_request_size)) {
            return protocol_error("invalid bulk string length");
        }
        const auto size = static_cast<std::size_t>(length);
        if (payload_start > max_request_size || size > max_request_size - payload_start ||
            max_request_size - payload_start - size < 2) {
            return protocol_error("request exceeds 1 MiB");
        }
        if (payload_start > bytes.size() || size > bytes.size() - payload_start) return {};
        if (bytes.size() - payload_start - size < 2) return {};
        if (bytes.substr(payload_start + size, 2) != "\r\n") {
            return protocol_error("bulk string missing CRLF");
        }
        request.arguments.emplace_back(bytes.substr(payload_start, size));
        position = payload_start + size + 2;
    }
    if (position > max_request_size) return protocol_error("request exceeds 1 MiB");
    return {ParseStatus::complete, std::move(request), position, {}};
}

std::string encode(const CommandResult& result) {
    return std::visit([](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, SimpleString>) return "+" + value.value + "\r\n";
        if constexpr (std::is_same_v<T, ErrorString>) return "-" + value.value + "\r\n";
        if constexpr (std::is_same_v<T, Integer>) return ":" + std::to_string(value.value) + "\r\n";
        if constexpr (std::is_same_v<T, BulkString>) {
            return "$" + std::to_string(value.value.size()) + "\r\n" + value.value + "\r\n";
        }
        return "$-1\r\n";
    }, result);
}
} // namespace mini_redis
