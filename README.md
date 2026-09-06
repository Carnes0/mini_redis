# Mini Redis

一个面向软件工程课程的 C++20 轻量级内存键值数据库。当前 MVP 支持 RESP2 子集，可使用 `redis-cli` 连接，提供 `PING`、`SET`、`GET`、`DEL` 和 `EXISTS`。

课程资料从 [文档导航](docs/README.md) 开始阅读；提交前查看 [交付清单](docs/delivery.md)。2026-09-06 本机 GCC/MSVC Release 构建和各 4 项单元测试已通过，最终验收仍待独立环境复现，详见 [正式测试报告](docs/reports/2026-09-06-test-report.md)。

## 功能与限制

- TCP 服务端，默认只监听 `127.0.0.1:6380`。
- 固定 4 工作线程，最多同时服务 4 个长连接；超出时返回 `BUSY`，避免连接在队列中无限等待。
- 线程安全的 String 键值存储。
- 支持 TCP 半包、粘包和同一连接连续命令。
- 单次未解析请求限制为 1 MiB。
- 数据仅存在内存中；重启后丢失。
- 首版不支持 TTL、持久化、事务、集群和 String 之外的数据类型。

## 环境要求

- Windows 10/11 x64
- CMake 3.20 或更高版本
- Visual Studio 2022（Desktop development with C++），或 MSYS2 UCRT64 GCC 与 `mingw32-make`
- 可选：`redis-cli`，用于兼容性演示

## 构建

在仓库根目录运行，选择一种工具链。分别使用独立目录，避免已有 GCC 缓存与 Visual Studio 生成器冲突。

### Visual Studio 2022

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Release
ctest --test-dir build-msvc -C Release --output-on-failure
.\build-msvc\Release\mini_redis.exe --port 6380
```

本机已验收的 MSVC 产物位于 `build-msvc-acceptance\Release\mini_redis.exe`；上面使用 `build-msvc` 作为常规开发目录。若改用 Debug，构建、CTest 和启动路径中的配置名须同步修改。

### MSYS2 UCRT64 GCC

确保 UCRT64 的 `bin` 在 PATH 中（本机路径为 `C:\msys64\ucrt64\bin`）：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build -C Release --output-on-failure
.\build\mini_redis.exe --port 6380
```

本机当前 `build` 为 GCC Release。新目录 GCC 复现产物位于 `build-acceptance`。

### 无 CMake 的备用编译

仅生成服务器，不替代 CMake/CTest 验收：

```powershell
New-Item -ItemType Directory -Path build-manual -Force
g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude `
  src/main.cpp src/server/tcp_server.cpp src/server/thread_pool.cpp `
  src/protocol/resp.cpp src/commands/dispatcher.cpp src/storage/store.cpp `
  -lws2_32 -o build-manual/mini_redis.exe
```

## 运行

使用所选构建步骤末尾的启动命令，在另一个终端执行客户端操作。服务器前台运行时不会返回命令提示符。

如端口被占用，可将 `6380` 换成 `1` 到 `65535` 之间的空闲端口。

## redis-cli 演示

```powershell
redis-cli -p 6380 PING
redis-cli -p 6380 SET course software-engineering
redis-cli -p 6380 GET course
redis-cli -p 6380 EXISTS course
redis-cli -p 6380 DEL course
```

预期依次输出：`PONG`、`OK`、`software-engineering`、`1`、`1`。

本机便携客户端位于 `build\redis-client\Redis-8.10.1-Windows-x64-msys2`。在新 PowerShell 窗口中，先执行以下命令，再运行上述演示或兼容性脚本：

```powershell
$env:Path = (Resolve-Path .\build\redis-client\Redis-8.10.1-Windows-x64-msys2).Path + ';' + $env:Path
```

客户端来自 [Redis Windows 社区移植版](https://github.com/redis-windows/redis-windows/releases/tag/8.10.1)，仅解压在忽略的构建目录中；清理 `build` 后需重新下载。

## 自动化测试

```powershell
ctest --test-dir build -C Release --output-on-failure
```

以上对应 GCC；MSVC 使用 `--test-dir build-msvc -C Release`。完整说明见 [测试导航](tests/README.md)。CTest 只运行四项单元测试，以下集成测试需另行执行。

安装 `redis-cli` 后，在服务运行期间执行：

```powershell
powershell -ExecutionPolicy Bypass -File tests\integration\redis_cli.ps1
```

不依赖 `redis-cli` 的持续连接测试（五条命令复用同一个 TCP 连接）：

```powershell
powershell -ExecutionPolicy Bypass -File tests\integration\persistent_resp.ps1 -Port 6380
```

## 目录

```text
include/mini_redis/   公共接口
src/protocol/        RESP2 编解码
src/storage/         线程安全存储
src/commands/        命令分发与业务语义
src/server/          线程池与 WinSock2 服务
tests/               单元和集成测试
docs/                软件工程项目文档
docs/reports/        正式报告与已归档证据
build*/             本地构建、下载和临时测试产物（不提交）
```

根目录原有五个旧 EXE 已归档到 `build/legacy-root/`，不作为当前运行入口。历史脚本快照保留在报告证据目录；初始设计和实施计划保留在 `docs/superpowers/`。

## 常见问题

- 生成器不匹配：使用对应目录，或另建构建目录。
- 脚本被执行策略阻止：使用上面的 `powershell -ExecutionPolicy Bypass -File` 命令。
- MSBuild 报 `Path/PATH` 重复键：本机处理过程与临时环境包装脚本见正式测试报告第 8 节及附录。
- `redis-cli` 找不到：安装客户端或使用上文的本机便携路径；持续连接脚本无需它。
- 第五个长连接收到 `BUSY`：当前最多 4 个活跃客户端，关闭闲置测试连接后重试。

## 课程文档

- [需求与用例](docs/requirements.md)
- [架构设计](docs/architecture.md)
- [项目计划（Gantt 与 PERT）](docs/project-plan.md)
- [测试与验收](docs/testing.md)
- [测试总结与阶段验收报告（2026-09-06）](docs/reports/2026-09-06-test-report.md)
- [完整设计规格](docs/superpowers/specs/2026-09-04-mini-redis-design.md)
- [详细实施计划](docs/superpowers/plans/2026-09-04-mini-redis-scaffold.md)
