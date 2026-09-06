#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace mini_redis {
struct SimpleString { std::string value; };
struct ErrorString { std::string value; };
struct Integer { std::int64_t value; };
struct BulkString { std::string value; };
struct NullBulkString {};

using CommandResult = std::variant<SimpleString, ErrorString, Integer, BulkString, NullBulkString>;
} // namespace mini_redis

