#include "mini_redis/server/thread_pool.hpp"
#include "test_support.hpp"

#include <atomic>

int main() {
    std::atomic<int> completed{0};
    {
        mini_redis::ThreadPool pool(2);
        for (int i = 0; i < 20; ++i) {
            pool.submit([&] { ++completed; });
        }
    }
    CHECK_EQ(completed.load(), 20);
    return test::finish();
}

