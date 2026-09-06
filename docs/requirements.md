# Mini Redis 需求规格

## 项目目标

两人用一周完成一个可由 `redis-cli` 连接的 C++20 内存键值数据库 MVP，用于展示网络协议、并发控制、分层设计、测试和项目管理实践。

## 功能需求

| 编号 | 功能 | 验收标准 |
|---|---|---|
| FR-01 | TCP 服务 | 多个客户端可连接 `127.0.0.1:6380` |
| FR-02 | RESP2 子集 | 正确处理 Array 与 Bulk String 请求 |
| FR-03 | `PING [message]` | 返回 `PONG` 或原消息 |
| FR-04 | `SET key value` | 保存值并返回 `OK` |
| FR-05 | `GET key` | 返回值或 Null Bulk String |
| FR-06 | `DEL key [key ...]` | 返回实际删除数量 |
| FR-07 | `EXISTS key [key ...]` | 返回存在的键数量 |
| FR-08 | 错误处理 | 未知命令和错误参数返回 `-ERR` |

## 非功能需求

- NFR-01：Windows 10/11 x64、MSVC 2022 和 C++20 环境可构建。
- NFR-02：存储支持多读单写，单条命令线程安全。
- NFR-03：单连接错误不导致整个进程退出。
- NFR-04：未解析请求不超过 1 MiB。
- NFR-05：协议、存储和命令模块可脱离真实 Socket 测试。
- NFR-06：全新环境可按 README 复现构建和测试。

## 主要用例

1. 用户执行 `SET course software-engineering`，随后 `GET course` 得到相同值。
2. 用户使用 `EXISTS` 检查键，再使用 `DEL` 删除并取得删除数量。
3. 用户输入未知命令，服务返回错误，连接仍可继续执行合法命令。
4. 客户端发送无法恢复的 RESP 帧，服务返回协议错误并只关闭该连接。

## 范围边界

首版只支持 String 和五个命令。TTL、AOF、更多数据类型、Reactor、事务、复制及集群属于后续迭代。

