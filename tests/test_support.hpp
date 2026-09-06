#pragma once

#include <iostream>
#include <string_view>

namespace test {
inline int failures = 0;

inline void check(bool ok, std::string_view expression, std::string_view file, int line) {
    if (!ok) {
        ++failures;
        std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
    }
}

inline int finish() { return failures == 0 ? 0 : 1; }
} // namespace test

#define CHECK(expr) ::test::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) CHECK((actual) == (expected))

