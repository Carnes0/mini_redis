#include "mini_redis/storage/store.hpp"
#include "test_support.hpp"

#include <array>
#include <string>
#include <thread>
#include <vector>

int main() {
    mini_redis::KeyValueStore store;
    CHECK(!store.get("missing").has_value());
    store.set("course", "software-engineering");
    CHECK_EQ(store.get("course").value(), "software-engineering");

    std::array<std::string, 2> keys{"course", "missing"};
    CHECK_EQ(store.count_existing(keys), 1U);
    CHECK_EQ(store.erase(keys), 1U);
    CHECK(!store.get("course").has_value());

    std::vector<std::jthread> writers;
    for (int i = 0; i < 8; ++i) {
        writers.emplace_back([&, i] { store.set("shared", std::to_string(i)); });
    }
    writers.clear();
    CHECK(store.get("shared").has_value());
    return test::finish();
}

