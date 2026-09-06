#pragma once

#include <optional>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mini_redis {
class KeyValueStore {
public:
    void set(std::string key, std::string value);
    [[nodiscard]] std::optional<std::string> get(std::string_view key) const;
    std::size_t erase(std::span<const std::string> keys);
    [[nodiscard]] std::size_t count_existing(std::span<const std::string> keys) const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> values_;
};
} // namespace mini_redis

