#pragma once

#include "mini_redis/common/result.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mini_redis {
inline constexpr std::size_t max_request_size = 1024U * 1024U;

struct CommandRequest { std::vector<std::string> arguments; };
enum class ParseStatus { complete, incomplete, protocol_error };
struct ParseResult {
    ParseStatus status{ParseStatus::incomplete};
    CommandRequest request;
    std::size_t consumed{};
    std::string error;
};

class RespParser {
public:
    [[nodiscard]] ParseResult parse(std::string_view bytes) const;
};

[[nodiscard]] std::string encode(const CommandResult& result);
} // namespace mini_redis

