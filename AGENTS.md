# Repository Guidelines

## Project Structure & Module Organization

This repository contains a Windows-focused C++20 Mini Redis implementation. Public interfaces live under `include/mini_redis/`; implementations are grouped by responsibility under `src/`: `protocol` handles RESP2, `storage` owns the thread-safe key-value map, `commands` validates and dispatches commands, and `server` contains WinSock2 and the worker pool. Unit tests are in `tests/`, while end-to-end PowerShell checks are in `tests/integration/`. Course requirements, architecture, project planning, and test records are maintained in `docs/`.

## Build, Test, and Development Commands

Use an out-of-tree CMake build:

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Run the server with `build\Debug\mini_redis.exe --port 6380`. With the server running, execute `tests\integration\persistent_resp.ps1 -Port 6380` to test five commands over one connection. If `redis-cli` is installed, run `tests\integration\redis_cli.ps1` for compatibility testing.

## Coding Style & Naming Conventions

Use four-space indentation and C++20. Keep headers in the matching `include/mini_redis/<module>/` directory and source files in `src/<module>/`. Classes use `PascalCase` (`KeyValueStore`); functions, variables, and files use `snake_case` (`count_existing`, `thread_pool.cpp`); private fields end in `_`. Prefer RAII, standard-library types, `std::string_view` for non-owning input, and explicit module boundaries. Compile with `/W4 /permissive-` on MSVC or `-Wall -Wextra -Wpedantic` on GCC.

## Testing Guidelines

Tests use the dependency-free assertions in `tests/test_support.hpp` and are registered with CTest. Name new unit files `<module>_test.cpp`. Add tests before implementation, including success, malformed input, boundary, and concurrency cases where relevant. Do not record an integration check as passed unless it was actually executed.

## Commit & Pull Request Guidelines

Use concise Conventional Commit prefixes such as `feat:`, `fix:`, `test:`, `docs:`, and `build:`. Pull requests should describe behavior changes, list commands run and results, link the relevant requirement ID (for example, `FR-05`), and update affected documentation. Include screenshots only when rendered diagrams or other visual documentation changes.

## Security & Configuration Tips

The MVP binds to `127.0.0.1` by default; do not expose it publicly. Keep the 1 MiB request limit and validate port values. Never commit build artifacts, executables, IDE state, or `.superpowers/` content.
