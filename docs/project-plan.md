# Mini Redis 项目管理计划

本文保留原定双人七天计划，图中的 2026-09-07 起始日期是排期基线，并非实际执行时间。实际开发与测试已提前开展，当前验收状态以 [测试报告](reports/2026-09-06-test-report.md) 为准；未据此虚构成员实际工时。

## WBS 与双人分工

| WBS | 工作 | 负责人 | 工期 | 前置 |
|---|---|---|---:|---|
| 1.1 | 需求分析与验收标准 | 共同 | 0.5d | — |
| 1.2 | 架构、接口与测试设计 | 共同 | 0.5d | 1.1 |
| 2.1 | RESP2 编解码 | A | 1d | 1.2 |
| 2.2 | TCP 服务与线程池 | A | 1d | 2.1 |
| 3.1 | 线程安全存储 | B | 0.75d | 1.2 |
| 3.2 | 命令分发与处理 | B | 0.75d | 3.1 |
| 4.1 | 模块集成 | 共同 | 1d | 2.2、3.2 |
| 4.2 | 系统与并发测试 | 共同 | 1d | 4.1 |
| 5.1 | 文档、验收与修复 | 共同 | 1d | 4.2 |
| M1 | 演示、打包与缓冲 | 共同 | 1d | 5.1 |

## Gantt 图

```mermaid
gantt
    title Mini Redis 双人七天开发计划
    dateFormat YYYY-MM-DD
    axisFormat D%d
    section 共同设计
    1.1 需求与验收 :crit, a1, 2026-09-07, 0.5d
    1.2 架构与测试设计 :crit, a2, after a1, 0.5d
    section 成员 A
    2.1 RESP2 编解码 :crit, a3, after a2, 1d
    2.2 TCP 与线程池 :crit, a4, after a3, 1d
    section 成员 B
    3.1 线程安全存储 :b1, after a2, 0.75d
    3.2 命令分发 :b2, after b1, 0.75d
    section 共同交付
    4.1 模块集成 :crit, c1, after a4, 1d
    4.2 系统测试 :crit, c2, after c1, 1d
    5.1 文档与修复 :crit, c3, after c2, 1d
    演示与缓冲 :c4, after c3, 1d
    M1 项目交付 :milestone, m1, after c4, 0d
```

`4.1` 同时依赖成员 A 与 B 两条分支；Gantt 使用较晚完成的 A 分支定位，双依赖由 PERT 表达。

## PERT/CPM 图

```mermaid
flowchart LR
    E1((1<br/>0/0)) -->|1.1 0.5d| E2((2<br/>0.5/0.5))
    E2 -->|1.2 0.5d| E3((3<br/>1/1))
    E3 -->|2.1 1d| E4((4<br/>2/2))
    E4 -->|2.2 1d| E6((6<br/>3/3))
    E3 -->|3.1 0.75d| E5((5<br/>1.75/2.25))
    E5 -->|3.2 0.75d| E7((7<br/>2.5/3))
    E6 --> E8((8<br/>3/3))
    E7 --> E8
    E8 -->|4.1 1d| E9((9<br/>4/4))
    E9 -->|4.2 1d| E10((10<br/>5/5))
    E10 -->|5.1 1d| M1{M1<br/>6/6}
    classDef critical fill:#fff1f2,stroke:#dc2626,stroke-width:3px;
    classDef normal fill:#eff6ff,stroke:#2563eb,stroke-width:2px;
    class E1,E2,E3,E4,E6,E8,E9,E10,M1 critical;
    class E5,E7 normal;
```

关键路径为 `1.1 → 1.2 → 2.1 → 2.2 → 4.1 → 4.2 → 5.1`，长度 6 天；成员 B 分支总时差 0.5 天，第 7 天为交付缓冲。

## 主要风险

- RESP 半包/粘包：先写增量解析测试，再接入 Socket。
- 接口不一致：第 1 天冻结请求、结果和存储接口。
- 功能蔓延：首版只接受五个命令和 String。
- 并发错误：存储层集中加锁并重复运行并发测试。
- 文档失真：交付前按需求编号逐项复核。
