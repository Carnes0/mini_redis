#pragma once

#include "mini_redis/protocol/resp.hpp"
#include "mini_redis/storage/store.hpp"

namespace mini_redis {
class CommandDispatcher {
public:
    explicit CommandDispatcher(KeyValueStore& store) : store_(store) {}
    [[nodiscard]] CommandResult execute(const CommandRequest& request) const;

private:
    KeyValueStore& store_;
};
} // namespace mini_redis

