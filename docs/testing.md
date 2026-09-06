# Mini Redis 测试与验收

正式报告：[2026-09-06 测试总结与阶段验收报告](reports/2026-09-06-test-report.md)，包含用例编号、需求追踪、异常处理、退出准则及归档证据。

## 测试矩阵

| 层次 | 覆盖内容 | 自动化入口 |
|---|---|---|
| 协议单元测试 | 完整帧、半包、畸形帧、五类响应编码 | `protocol_test` |
| 存储单元测试 | 基本读写、删除、存在性、并发写后键存在 | `storage_test` |
| 命令单元测试 | 五个命令、大小写、参数错误、未知命令、错误信息注入 | `commands_test` |
| 并发设施测试 | 任务执行和析构前排空队列 | `thread_pool_test` |
| 端到端测试 | 同一 TCP 连接执行五个命令 | `persistent_resp.ps1` |
| 客户端兼容测试 | 使用真实客户端分别执行五个命令 | `redis_cli.ps1` |
| 扩展网络验收 | 空值、二进制、覆盖、请求上限、连接容量与并发 | 正式报告附录中的脚本快照 |

## 2026-09-04 本机验收记录

- 编译器：MSYS2 UCRT64 `g++`，C++20。
- 编译选项：`-Wall -Wextra -Wpedantic`。
- 单元测试：`protocol_test`、`storage_test`、`commands_test`、`thread_pool_test` 全部通过。
- 服务：成功监听 `127.0.0.1:6380`。
- 原始 RESP2 TCP 验收结果：`+PONG`、`+OK`、`$20 software-engineering`、`:1`、`:1`。
- `persistent_resp.ps1`：通过；五条命令复用同一个 TCP 连接。
- `redis-cli`：本机未安装，因此脚本已提供但未在本机执行；不能将其记录为已通过。
- CMake：本机命令不可用，因此 CMake 配置已编写但未在本机执行；需在课程验收环境补做。

## 2026-09-06 本机补充验收记录

- 环境：Windows，CMake 4.4.3，MSYS2 UCRT64 GCC 15.2.0。
- 配置：`cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release` 成功。
- 构建：`cmake --build build --parallel 4` 成功。
- 单元测试：`ctest --test-dir build -C Release --output-on-failure`，4/4 通过。
- 服务：本次构建的 `build\mini_redis.exe --port 6380`。
- 持续连接：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/integration/persistent_resp.ps1 -Port 6380` 通过。
- 客户端兼容：便携版 `redis-cli 8.10.1`；加入当前进程 PATH 后，`powershell -NoProfile -ExecutionPolicy Bypass -File tests/integration/redis_cli.ps1` 通过。
- 畸形请求：通过 TCP 发送 `!bad\r\n`，收到 `-ERR Protocol error:`；随后重新执行持续连接测试通过，确认服务继续工作。
- 客户端来源：https://github.com/redis-windows/redis-windows/releases/tag/8.10.1 ，使用无 Service 的 MSYS2 ZIP。
- 下载包 SHA256 与 GitHub 发布 API 提供的摘要一致：`4e8f2f956ed92feadf3f64b4e137ed34026438821e692e7ae22c9bba5976607a`。
- 本次补齐了此前未执行的 CMake 和真实客户端检查；另一名成员的独立复现仍待完成。

## 2026-09-06 扩展验收与双编译器复现

本轮未修改产品源码。以下检查均实际执行：

| 检查 | 结果 |
|---|---|
| 全新 `build-acceptance` 目录，GCC 15.2.0 Release 构建 | 通过；CTest 4/4 |
| 独立 `build-msvc-acceptance` 目录，VS 2022 / MSVC 19.44.35207.1 x64 Release 构建 | 通过；CTest 4/4 |
| 非法端口和命令行参数，共 8 例 | 均以退出码 2 拒绝 |
| 逐字节发送请求、多条请求一次发送 | 响应内容和顺序正确 |
| 不存在的键、空值、包含全部 256 种字节及中文的值 | 读写结果正确 |
| 未知命令、参数错误后在同一连接执行 PING | 返回 ERR 后仍正常响应 |
| 完整请求恰好 1 MiB | SET 成功，GET 内容一致 |
| 完整请求超过 1 MiB 一个字节、超限 Bulk 长度、非法 Array/Bulk 和前缀 | 返回 ERR 并关闭该连接 |
| 4 个活跃连接，第 5 个连接 | 前 4 个正常，第 5 个收到 BUSY 并关闭 |
| 连接释放后重新连接，4 客户端各执行 100 组 SET/GET | 800 条命令全部通过 |

以上网络和参数检查分别在 GCC 与 MSVC 可执行文件上运行通过，测试进程均已停止。

本机执行入口（临时验收脚本在忽略的构建目录中，清理构建目录后不保留）：

```powershell
python build-acceptance/acceptance.py
python build-acceptance/acceptance.py build-msvc-acceptance/Release/mini_redis.exe
```

MSVC 初次配置失败的根因是当前工具进程环境同时包含 `Path` / `PATH`，触发 MSBuild 的 MSB6001 重复键错误。通过 Python `dict(os.environ)` 构造去重的子进程环境后，重新配置、构建及 CTest 均通过；未更改系统环境变量。实际命令为：

```powershell
cmake --fresh -S . -B build-msvc-acceptance -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc-acceptance --config Release --parallel 4
ctest --test-dir build-msvc-acceptance -C Release --output-on-failure
```

范围限制：这是同一台机器上的新目录复现，不等于另一名成员或全新机器验收。NFR-06 的独立环境复现仍待完成；本轮未覆盖 Ctrl+C 优雅退出、长时间压力和内存泄漏检测。超限请求关闭时可能收到 WinSock TCP reset，验收允许 EOF 或 reset，但要求先收到 ERR。

## 最终退出准则

1. CMake Release 构建成功。
2. CTest 显示 100% 测试通过。
3. `redis-cli` 五条命令与 README 预期一致。
4. 畸形请求不会导致整个服务退出。
5. 另一名成员可从全新构建目录复现结果。
