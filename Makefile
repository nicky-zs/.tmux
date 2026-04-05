# tmux resource usage - Makefile

# ==================== 编译器与标志 ====================
CC      := gcc
CFLAGS  := -O3 -DNDEBUG -Wall -Wextra
LDFLAGS := -lm

# ==================== 目录结构 ====================
SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := bin

# ==================== 源文件与目标 ====================
SRCS := $(wildcard $(SRC_DIR)/*.c)

# 生产目标
RESOURCE_USAGE := $(BUILD_DIR)/resource_usage

# 测试目标
TEST_BINS := $(TEST_DIR)/test_cpu_runner \
             $(TEST_DIR)/test_mem_runner \
             $(TEST_DIR)/test_display_runner

# ==================== 默认目标 ====================
.PHONY: all
all: $(RESOURCE_USAGE)

# ==================== 构建规则 ====================
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(RESOURCE_USAGE): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

# ==================== 测试目标 ====================
.PHONY: test tests run-tests check
test: tests
tests: $(TEST_BINS)

# 测试编译（排除 main.c）
$(TEST_DIR)/test_cpu_runner: $(TEST_DIR)/test_cpu.c $(filter-out $(SRC_DIR)/main.c,$(SRCS))
	$(CC) $(CFLAGS) -DUNIT_TEST $^ -o $@ $(LDFLAGS)

$(TEST_DIR)/test_mem_runner: $(TEST_DIR)/test_mem.c $(filter-out $(SRC_DIR)/main.c,$(SRCS))
	$(CC) $(CFLAGS) -DUNIT_TEST $^ -o $@ $(LDFLAGS)

$(TEST_DIR)/test_display_runner: $(TEST_DIR)/test_display.c $(filter-out $(SRC_DIR)/main.c,$(SRCS))
	$(CC) $(CFLAGS) -DUNIT_TEST $^ -o $@ $(LDFLAGS)

run-tests: tests
	@echo "========================================"
	@echo "运行单元测试..."
	@echo "========================================"
	@for test in $(TEST_BINS); do \
		echo ""; \
		$$test || exit 1; \
	done
	@echo ""
	@echo "========================================"
	@echo "所有测试通过！"
	@echo "========================================"

check: run-tests

# ==================== 清理目标 ====================
.PHONY: clean distclean
clean:
	rm -f $(TEST_BINS)
	rm -f $(RESOURCE_USAGE)
	rm -rf $(BUILD_DIR)
	rm -f /tmp/test_display_output
	rm -f /tmp/test_resource_*

distclean: clean
	rm -f *~ $(SRC_DIR)/*~ $(TEST_DIR)/*~

# ==================== 帮助信息 ====================
.PHONY: help
help:
	@echo "tmux resource usage - 可用目标:"
	@echo ""
	@echo "  make / make all   - 编译生产版本 (默认，O3 优化)"
	@echo "  make test         - 编译所有测试"
	@echo "  make run-tests    - 运行所有测试"
	@echo "  make clean        - 清理编译产物"
	@echo "  make distclean    - 清理所有生成文件"
	@echo "  make help         - 显示此帮助信息"
	@echo ""
	@echo "示例:"
	@echo "  make                              # 编译到 bin/resource_usage"
	@echo "  make test && make run-tests       # 运行测试"
	@echo "  make clean                        # 清理"
