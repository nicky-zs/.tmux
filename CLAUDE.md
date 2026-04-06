# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

tmux 自定义配置和资源监控工具。包含：
- tmux 配置文件（`tmux.conf`）
- 自研的 CPU/内存利用率监控工具（C 语言实现）

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
  - `resource-usage.c` - 主逻辑和显示输出
  - `main.c` - 程序入口
- `tests/` - 单元测试（使用 `test_harness.h` 断言框架）
- `scripts/` - Shell 脚本（状态栏左右侧显示脚本）
- `bin/` - 编译产物

### 测试框架

测试文件通过 `#include "test_harness.h"` 提供断言宏：
- `ASSERT_TRUE(cond)` / `ASSERT_FALSE(cond)`
- `ASSERT_EQ(expected, actual)`
- `ASSERT_DOUBLE_EQ(expected, actual, epsilon)`
- `ASSERT_STR_EQ(expected, actual)`

测试编译时使用 `-DUNIT_TEST` 标志。

### 显示逻辑

状态栏颜色根据 CPU 利用率动态变化（`get_color_for_rate` 函数）：
- < 50%: green
- < 75%: yellow  
- < 85%: colour208
- >= 85%: red

## 特殊说明

- CPU 临时文件路径由 `TMUX` 环境变量派生，位于 `/tmp/test_resource_${session_id}`
- 仅支持 Linux（依赖 `/proc/stat` 和 `/proc/meminfo`）
- 生产二进制文件：`bin/resource_usage`
