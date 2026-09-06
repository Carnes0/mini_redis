#include "mini_redis/storage/store.hpp"

#include <mutex>

namespace mini_redis {
void KeyValueStore::set(std::string key, std::string value) {
    std::unique_lock lock(mutex_);
    values_.insert_or_assign(std::move(key), std::move(value));
}

std::optional<std::string> KeyValueStore::get(std::string_view key) const {
    std::shared_lock lock(mutex_);
    const auto found = values_.find(std::string(key));
    if (found == values_.end()) return std::nullopt;
    return found->second;
}

std::size_t KeyValueStore::erase(std::span<const std::string> keys) {
    std::unique_lock lock(mutex_);
    std::size_t erased = 0;
    for (const auto& key : keys) erased += values_.erase(key);
    return erased;
}

std::size_t KeyValueStore::count_existing(std::span<const std::string> keys) const {
    std::shared_lock lock(mutex_);
    std::size_t count = 0;
    for (const auto& key : keys) count += values_.contains(key) ? 1U : 0U;
    return count;
}
} // namespace mini_redis

