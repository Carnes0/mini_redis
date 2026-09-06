# Mini Redis Scaffold Implementation Plan

> 历史实施计划：保留原任务与复选框，不作为当前完成状态。实际构建、测试和未完成验收事项见 [正式测试报告](../../reports/2026-09-06-test-report.md)；当前文档入口见 [文档导航](../../README.md)。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a C++20 Mini Redis scaffold on Windows that compiles with CMake, accepts RESP2 requests from `redis-cli`, implements `PING`, `SET`, `GET`, `DEL`, and `EXISTS`, and includes automated tests and software-engineering documentation.

**Architecture:** Use a layered monolith. WinSock2 and the fixed thread pool live in the server module; incremental RESP2 parsing lives in protocol; command validation and dispatch live in commands; a `shared_mutex`-protected String map lives in storage. Business modules do not depend on sockets and are tested independently.

**Tech Stack:** C++20, CMake 3.20+, MSVC 2022, WinSock2, CTest, PowerShell, redis-cli.

---

## File map

```text
CMakeLists.txt                         root build and CTest registration
README.md                              reproducible build/run/demo guide
include/mini_redis/common/result.hpp   shared RESP response value types
include/mini_redis/protocol/resp.hpp   parser/encoder public API
src/protocol/resp.cpp                  incremental RESP2 implementation
include/mini_redis/storage/store.hpp   thread-safe String store API
src/storage/store.cpp                  shared_mutex-protected operations
include/mini_redis/commands/dispatcher.hpp command API
src/commands/dispatcher.cpp            five command implementations
include/mini_redis/server/thread_pool.hpp fixed worker pool API
src/server/thread_pool.cpp             worker queue implementation
include/mini_redis/server/tcp_server.hpp Windows TCP server API
src/server/tcp_server.cpp              WinSock2 accept/session loop
src/main.cpp                           CLI configuration and process entry
tests/test_support.hpp                 dependency-free assertions
tests/protocol_test.cpp                RESP parser/encoder tests
tests/storage_test.cpp                 store and concurrency tests
tests/commands_test.cpp                command behavior tests
docs/requirements.md                   requirements and use cases
docs/architecture.md                   component and data-flow design
docs/project-plan.md                   Gantt, PERT, WBS, division and risks
docs/testing.md                        test cases and acceptance record
```

### Task 1: Establish the build and test harness

**Files:**
- Create: `CMakeLists.txt`
- Create: `tests/test_support.hpp`
- Create: `tests/smoke_test.cpp`

- [ ] **Step 1: Write the failing smoke test**

```cpp
#include "test_support.hpp"

int main() {
    CHECK_EQ(2 + 2, 4);
    return test::finish();
}
```

- [ ] **Step 2: Configure to verify the missing build fails**

Run: `cmake -S . -B build`

Expected: FAIL because the root `CMakeLists.txt` does not exist.

- [ ] **Step 3: Add a dependency-free assertion helper**

```cpp
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
}

#define CHECK(expr) ::test::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) CHECK((actual) == (expected))
```

- [ ] **Step 4: Add the initial CMake project**

```cmake
cmake_minimum_required(VERSION 3.20)
project(mini_redis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
add_executable(smoke_test tests/smoke_test.cpp)
target_include_directories(smoke_test PRIVATE tests)
add_test(NAME smoke_test COMMAND smoke_test)
```

- [ ] **Step 5: Build and run the smoke test**

Run: `cmake -S . -B build; cmake --build build --config Debug; ctest --test-dir build -C Debug --output-on-failure`

Expected: configuration and build succeed; `1/1 Test #1: smoke_test ... Passed`.

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt tests/test_support.hpp tests/smoke_test.cpp
git commit -m "build: establish CMake and CTest harness"
```

### Task 2: Implement the thread-safe String store

**Files:**
- Create: `include/mini_redis/storage/store.hpp`
- Create: `src/storage/store.cpp`
- Create: `tests/storage_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing store tests**

```cpp
#include "mini_redis/storage/store.hpp"
#include "test_support.hpp"
#include <array>

int main() {
    mini_redis::KeyValueStore store;
    CHECK(!store.get("missing").has_value());
    store.set("course", "software-engineering");
    CHECK_EQ(store.get("course").value(), "software-engineering");
    std::array<std::string, 2> keys{"course", "missing"};
    CHECK_EQ(store.count_existing(keys), 1U);
    CHECK_EQ(store.erase(keys), 1U);
    CHECK(!store.get("course").has_value());
    return test::finish();
}
```

- [ ] **Step 2: Build to verify the test fails**

Run: `cmake --build build --config Debug`

Expected: FAIL because `mini_redis/storage/store.hpp` is missing.

- [ ] **Step 3: Implement the store API and locking**

```cpp
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
```

Use `std::unique_lock` for `set`/`erase` and `std::shared_lock` for `get`/`count_existing`. Copy values out while the lock is held.

- [ ] **Step 4: Register and run storage tests**

Run: `cmake -S . -B build; cmake --build build --config Debug; ctest --test-dir build -C Debug -R storage --output-on-failure`

Expected: `storage_test` passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt include/mini_redis/storage/store.hpp src/storage/store.cpp tests/storage_test.cpp
git commit -m "feat: add thread-safe string store"
```

### Task 3: Implement incremental RESP2 parsing and encoding

**Files:**
- Create: `include/mini_redis/common/result.hpp`
- Create: `include/mini_redis/protocol/resp.hpp`
- Create: `src/protocol/resp.cpp`
- Create: `tests/protocol_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing protocol tests**

```cpp
mini_redis::RespParser parser;
auto complete = parser.parse("*2\r\n$4\r\nPING\r\n$5\r\nhello\r\n");
CHECK_EQ(complete.status, mini_redis::ParseStatus::complete);
CHECK_EQ(complete.request.arguments.at(1), "hello");
CHECK_EQ(complete.consumed, 25U);

auto partial = parser.parse("*2\r\n$4\r\nPING\r\n$5\r\nhel");
CHECK_EQ(partial.status, mini_redis::ParseStatus::incomplete);

auto malformed = parser.parse("!2\r\n");
CHECK_EQ(malformed.status, mini_redis::ParseStatus::protocol_error);
CHECK_EQ(mini_redis::encode(mini_redis::SimpleString{"PONG"}), "+PONG\r\n");
```

- [ ] **Step 2: Build to verify missing types fail**

Run: `cmake --build build --config Debug`

Expected: FAIL because RESP public headers are missing.

- [ ] **Step 3: Add explicit parse states and response variants**

```cpp
enum class ParseStatus { complete, incomplete, protocol_error };
struct CommandRequest { std::vector<std::string> arguments; };
struct ParseResult {
    ParseStatus status;
    CommandRequest request;
    std::size_t consumed{};
    std::string error;
};

struct SimpleString { std::string value; };
struct ErrorString { std::string value; };
struct Integer { std::int64_t value; };
struct BulkString { std::string value; };
struct NullBulkString {};
using CommandResult = std::variant<SimpleString, ErrorString, Integer, BulkString, NullBulkString>;
```

Implement strict CRLF checks, non-negative array/bulk lengths, incomplete-frame detection, a 1 MiB request limit, and visitor-based encoding.

- [ ] **Step 4: Run protocol tests**

Run: `cmake -S . -B build; cmake --build build --config Debug; ctest --test-dir build -C Debug -R protocol --output-on-failure`

Expected: complete, partial, malformed and encoding cases pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt include/mini_redis/common/result.hpp include/mini_redis/protocol/resp.hpp src/protocol/resp.cpp tests/protocol_test.cpp
git commit -m "feat: add incremental RESP2 codec"
```

### Task 4: Implement the command dispatcher

**Files:**
- Create: `include/mini_redis/commands/dispatcher.hpp`
- Create: `src/commands/dispatcher.cpp`
- Create: `tests/commands_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing command tests**

```cpp
mini_redis::KeyValueStore store;
mini_redis::CommandDispatcher commands(store);
CHECK_EQ(std::get<mini_redis::SimpleString>(commands.execute({{"PING"}})).value, "PONG");
CHECK_EQ(std::get<mini_redis::SimpleString>(commands.execute({{"set", "k", "v"}})).value, "OK");
CHECK_EQ(std::get<mini_redis::BulkString>(commands.execute({{"GET", "k"}})).value, "v");
CHECK_EQ(std::get<mini_redis::Integer>(commands.execute({{"EXISTS", "k", "x"}})).value, 1);
CHECK_EQ(std::get<mini_redis::Integer>(commands.execute({{"DEL", "k"}})).value, 1);
CHECK(std::holds_alternative<mini_redis::NullBulkString>(commands.execute({{"GET", "k"}})));
CHECK(std::holds_alternative<mini_redis::ErrorString>(commands.execute({{"UNKNOWN"}})));
```

- [ ] **Step 2: Build to verify missing dispatcher fails**

Run: `cmake --build build --config Debug`

Expected: FAIL because `CommandDispatcher` is missing.

- [ ] **Step 3: Implement dispatcher behavior**

Normalize only ASCII command names to uppercase. Validate these arities: `PING` 1–2, `SET` exactly 3, `GET` exactly 2, `DEL` at least 2, `EXISTS` at least 2. Empty requests return `ERR empty command`; unknown commands return `ERR unknown command`.

- [ ] **Step 4: Run command tests**

Run: `cmake -S . -B build; cmake --build build --config Debug; ctest --test-dir build -C Debug -R commands --output-on-failure`

Expected: all success, null and error response cases pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt include/mini_redis/commands/dispatcher.hpp src/commands/dispatcher.cpp tests/commands_test.cpp
git commit -m "feat: implement core command dispatcher"
```

### Task 5: Add the fixed thread pool

**Files:**
- Create: `include/mini_redis/server/thread_pool.hpp`
- Create: `src/server/thread_pool.cpp`
- Create: `tests/thread_pool_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing execution test**

```cpp
std::atomic<int> completed{0};
{
    mini_redis::ThreadPool pool(2);
    for (int i = 0; i < 20; ++i) pool.submit([&] { ++completed; });
}
CHECK_EQ(completed.load(), 20);
```

- [ ] **Step 2: Build to verify missing pool fails**

Run: `cmake --build build --config Debug`

Expected: FAIL because `ThreadPool` is missing.

- [ ] **Step 3: Implement bounded lifecycle behavior**

Use `std::vector<std::jthread>`, `std::queue<std::function<void()>>`, a mutex and condition variable. Reject construction with zero workers. `submit` throws after shutdown; the destructor requests shutdown, wakes workers and joins them after queued tasks finish.

- [ ] **Step 4: Run pool and full unit tests**

Run: `cmake -S . -B build; cmake --build build --config Debug; ctest --test-dir build -C Debug --output-on-failure`

Expected: all registered tests pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt include/mini_redis/server/thread_pool.hpp src/server/thread_pool.cpp tests/thread_pool_test.cpp
git commit -m "feat: add fixed worker thread pool"
```

### Task 6: Connect WinSock2 sessions to the core

**Files:**
- Create: `include/mini_redis/server/tcp_server.hpp`
- Create: `src/server/tcp_server.cpp`
- Create: `src/main.cpp`
- Create: `tests/integration/redis_cli.ps1`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing integration script**

```powershell
$ErrorActionPreference = 'Stop'
$pong = redis-cli -p 6380 PING
if ($pong -ne 'PONG') { throw "PING failed: $pong" }
redis-cli -p 6380 SET course software-engineering | Out-Null
$value = redis-cli -p 6380 GET course
if ($value -ne 'software-engineering') { throw "GET failed: $value" }
$exists = redis-cli -p 6380 EXISTS course
if ($exists -ne '1') { throw "EXISTS failed: $exists" }
$deleted = redis-cli -p 6380 DEL course
if ($deleted -ne '1') { throw "DEL failed: $deleted" }
```

- [ ] **Step 2: Run it to verify no service is available**

Run: `powershell -ExecutionPolicy Bypass -File tests/integration/redis_cli.ps1`

Expected: FAIL because port 6380 is not accepting connections.

- [ ] **Step 3: Implement the TCP server**

`TcpServer::run()` initializes WinSock2, binds `127.0.0.1:6380`, listens, and submits each accepted socket to the pool. A session owns its socket and receive buffer, repeatedly parses all complete requests, dispatches them and sends all encoded bytes. Protocol errors produce `ERR Protocol error` and close only that session. CMake links `ws2_32` only to the server target.

- [ ] **Step 4: Add a process entry point**

```cpp
int main(int argc, char** argv) {
    const auto port = parse_port_or_default(argc, argv, 6380);
    mini_redis::KeyValueStore store;
    mini_redis::CommandDispatcher dispatcher(store);
    mini_redis::TcpServer server(port, 4, dispatcher);
    return server.run();
}
```

- [ ] **Step 5: Build, launch and run integration acceptance**

Run server in terminal 1: `build\Debug\mini_redis.exe --port 6380`

Run in terminal 2: `powershell -ExecutionPolicy Bypass -File tests/integration/redis_cli.ps1`

Expected: script exits with code 0; the server remains available for another `redis-cli -p 6380 PING`.

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt include/mini_redis/server/tcp_server.hpp src/server/tcp_server.cpp src/main.cpp tests/integration/redis_cli.ps1
git commit -m "feat: serve RESP commands over WinSock2"
```

### Task 7: Produce course documentation

**Files:**
- Create: `README.md`
- Create: `docs/requirements.md`
- Create: `docs/architecture.md`
- Create: `docs/project-plan.md`
- Create: `docs/testing.md`

- [ ] **Step 1: Add the reproducibility checklist to README**

Include prerequisites, exact CMake commands, executable location for single- and multi-config generators, port override, `redis-cli` examples, supported commands, limitations, directory map and troubleshooting for port conflicts.

- [ ] **Step 2: Split the approved design into course-facing documents**

Copy requirements/use cases into `docs/requirements.md`; architecture/interfaces/data flow into `docs/architecture.md`; WBS/Gantt/PERT/division/risks into `docs/project-plan.md`; test matrix and exit criteria into `docs/testing.md`. Use the same requirement IDs and WBS numbers as the approved design spec.

- [ ] **Step 3: Verify documentation commands**

Run every README build and test command from a fresh `build-doc-check` directory.

Expected: build succeeds, CTest passes, and no path depends on a developer-specific absolute directory.

- [ ] **Step 4: Scan documentation for unfinished content**

Run: `rg -n "TBD|TODO|待定|待补充|example only" README.md docs`

Expected: no matches.

- [ ] **Step 5: Commit**

```powershell
git add README.md docs
git commit -m "docs: add software engineering project documentation"
```

### Task 8: Perform final quality and acceptance checks

**Files:**
- Modify: `docs/testing.md`

- [ ] **Step 1: Run a clean build**

Run: `cmake -S . -B build-final; cmake --build build-final --config Release`

Expected: `mini_redis.exe` and all test executables build without errors.

- [ ] **Step 2: Run the complete automated suite**

Run: `ctest --test-dir build-final -C Release --output-on-failure`

Expected: 100% tests passed.

- [ ] **Step 3: Run the redis-cli acceptance script**

Start `build-final\Release\mini_redis.exe --port 6380`, then run `powershell -ExecutionPolicy Bypass -File tests/integration/redis_cli.ps1`.

Expected: exit code 0 and all five commands return the documented RESP semantics.

- [ ] **Step 4: Record the acceptance result**

In `docs/testing.md`, record the date, MSVC/CMake versions, configuration, passed test count and the five observed redis-cli results. Do not claim an unexecuted test passed.

- [ ] **Step 5: Review the diff and commit**

Run: `git diff --check; git status --short`

Expected: no whitespace errors; only intended project files are pending.

```powershell
git add docs/testing.md
git commit -m "test: record final acceptance results"
```
