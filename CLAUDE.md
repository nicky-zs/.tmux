# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

tmux 自定义配置和资源监控工具。包含：
- tmux 配置文件（`tmux.conf`）
- 自研的 CPU/内存利用率监控工具（C 语言实现）
- 状态栏动态显示（资源栏 + 时间栏）

## 常用命令

```bash
# 编译生产版本（O3 优化）
make

# 运行测试
make test && make run-tests

# 清理
make clean
```

## 代码架构

### 目录结构

- `src/` - C 语言源码
  - `resource-usage.h` - 核心头文件，定义数据结构和函数接口
  - `cpu.c` - CPU 利用率计算（读取 `/proc/stat`）
  - `mem.c` - 内存利用率计算（读取 `/proc/meminfo`）
  - `resource-usage.c` - 主逻辑和显示输出（统一入口函数）
  - `main.c` - 程序入口
- `tests/` - 单元测试
  - `test_harness.h` - 断言框架
  - `test_cpu.c` - CPU 测试
  - `test_mem.c` - 内存测试
  - `test_display.c` - 显示格式化测试
  - `test_status_line.c` - 状态行穷举测试（54 种组合）
- `scripts/` - Shell 脚本
  - `status-right.sh` - 状态栏右侧（直接调用 resource_usage）
  - `status-left.sh` - 状态栏左侧
  - `helpers.sh` - 辅助函数
- `bin/` - 编译产物
- `docs/solutions/` - 经验总结文档

### 核心函数

**统一入口函数**（测试和生产共用）：
```c
// 生成完整状态行（资源栏 + 时间栏）
void format_full_status_line(const display_state *state, int wide_mode, char *buffer, size_t bufsize);

// 格式化时间栏
void format_time_segment(int wide_mode, char *buffer, size_t bufsize);
```

**纯函数**（可测试）：
```c
void format_status_line(const display_state *state, char *buffer, size_t bufsize);
void format_cpu_segment(const char *emoji, double rate, int narrow_mode, char *buffer, size_t bufsize);
void format_mem_segment(double rate, unsigned long long total_kb, int narrow_mode, char *buffer, size_t bufsize);
```

### 测试框架

测试文件通过 `#include "test_harness.h"` 提供断言宏：
- `ASSERT_TRUE(cond)` / `ASSERT_FALSE(cond)`
- `ASSERT_EQ(expected, actual)`
- `ASSERT_DOUBLE_EQ(expected, actual, epsilon)`
- `ASSERT_STR_EQ(expected, actual)`

测试编译时使用 `-DUNIT_TEST` 标志。

### 显示逻辑

状态栏颜色根据利用率动态变化（`color_code_for_rate` 函数）：
- < 50%: green (`colour10`)
- < 75%: yellow (`colour3`)  
- >= 85%: red (`colour1`)

### 输出格式

**宽屏模式**（>200 字符）：
```
 📊CPU% 📈CPU% ㎇used/total  Mon 2026/04/06 09:00:00 
```

**窄屏模式**（≤200 字符）：
```
CPU%|CPU%|used/total 04/06 09:00:00 
```

### 测试策略

**状态行穷举测试**（`test_status_line.c`）：
- 组合维度：[宽屏，窄屏] × CPU[低，中，高] × 单核 [低，中，高] × 内存 [低，中，高]
- 共 54 种组合
- 使用统一函数 `format_full_status_line()`，确保测试和生产一致
- ANSI 彩色输出，便于肉眼 review

## 特殊说明

- CPU 临时文件路径由 `TMUX` 环境变量派生，位于 `/tmp/test_resource_${session_id}`
- 仅支持 Linux（依赖 `/proc/stat` 和 `/proc/meminfo`）
- 生产二进制文件：`bin/resource_usage`
- **架构原则**：测试和生产必须走同一段代码（`format_full_status_line`）
