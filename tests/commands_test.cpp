#include "mini_redis/commands/dispatcher.hpp"
#include "test_support.hpp"

#include <variant>

int main() {
    mini_redis::KeyValueStore store;
    mini_redis::CommandDispatcher commands(store);
    CHECK_EQ(std::get<mini_redis::SimpleString>(commands.execute({{"PING"}})).value, "PONG");
    CHECK_EQ(std::get<mini_redis::BulkString>(commands.execute({{"PING", "hello"}})).value, "hello");
    CHECK_EQ(std::get<mini_redis::SimpleString>(commands.execute({{"set", "k", "v"}})).value, "OK");
    CHECK_EQ(std::get<mini_redis::BulkString>(commands.execute({{"GET", "k"}})).value, "v");
    CHECK_EQ(std::get<mini_redis::Integer>(commands.execute({{"EXISTS", "k", "x"}})).value, 1);
    CHECK_EQ(std::get<mini_redis::Integer>(commands.execute({{"DEL", "k"}})).value, 1);
    CHECK(std::holds_alternative<mini_redis::NullBulkString>(commands.execute({{"GET", "k"}})));
    CHECK(std::holds_alternative<mini_redis::ErrorString>(commands.execute({{"UNKNOWN"}})));
    const auto injected = std::get<mini_redis::ErrorString>(commands.execute({{"BAD\r\n+INJECTED"}}));
    CHECK_EQ(injected.value, "ERR unknown command");
    CHECK(std::holds_alternative<mini_redis::ErrorString>(commands.execute({{"SET", "only-key"}})));
    return test::finish();
}
