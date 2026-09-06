# 测试运行说明

## 单元测试

先按 [项目 README](../README.md) 构建。MSVC 使用 `ctest --test-dir build-msvc -C Release --output-on-failure`；GCC 使用 `ctest --test-dir build -C Release --output-on-failure`。

| 程序 | 现有检查 |
|---|---|
| protocol_test | 完整帧、半帧、非法前缀、拼接数据首帧解析和响应编码 |
| storage_test | 缺失键、基本读写、存在性、删除、并发写后键存在 |
| commands_test | 五命令、大小写、错误参数、未知命令及错误信息注入 |
| thread_pool_test | 析构返回前完成 20 个任务 |

四个程序使用 `test_support.hpp` 的断言，已由 CMake 注册到 CTest；CTest 不自动启动服务器或执行以下集成脚本。

## 集成测试

手动启动服务器后，在仓库根目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/integration/persistent_resp.ps1 -Port 6380
powershell -NoProfile -ExecutionPolicy Bypass -File tests/integration/redis_cli.ps1
```

第一项只依赖 PowerShell/.NET，五条命令复用一条连接。第二项要求 `redis-cli` 在 PATH 中，当前脚本固定连接 6380。两项会操作并删除 `course` 键，应使用本地测试实例；测试完后停止自己启动的服务。

## 已归档扩展验收

半包、粘包、二进制值、1 MiB 边界、BUSY 与四客户端并发检查使用过临时 Python 脚本。其不可变快照、原路径恢复方法及输入结果见 [正式测试报告](../docs/reports/2026-09-06-test-report.md)。快照不是 CTest 用例，也不是单独的执行日志。

报告保留了实际运行结果，本页只提供入口。未执行项不得记录为通过；后续新增测试应记录日期、工具链、输入、预期和实际输出。
