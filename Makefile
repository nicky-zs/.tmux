# tmux resource usage - Makefile

# ==================== 编译器与标志 ====================
CC      := gcc
CFLAGS  := -O3 -DNDEBUG -Wall -Wextra
LDFLAGS := -lm

# ==================== 目录结构 ====================
SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := bin
TEST_BUILD_DIR := $(BUILD_DIR)/tests

# ==================== 源文件与目标 ====================
SRCS := $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)

# 生产目标
RESOURCE_USAGE := $(BUILD_DIR)/resource_usage

# 测试源码（排除测试 harness 头文件）
TEST_C_FILES := $(filter-out $(TEST_DIR)/test_harness.h, $(TEST_SRCS))

# ==================== 默认目标 ====================
.PHONY: all
all: $(RESOURCE_USAGE)

# ==================== 构建规则 ====================
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(RESOURCE_USAGE): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

# ==================== 测试目标 ====================
# 编译并立即运行测试，运行后删除产物
.PHONY: test check view-test
test: check
check:
	@echo "========================================"
	@echo "运行单元测试..."
	@echo "========================================"
	@mkdir -p $(TEST_BUILD_DIR)
	@failed=0; \
	for test_src in $(TEST_C_FILES); do \
		test_name=$$(basename $$test_src .c); \
		test_bin=$(TEST_BUILD_DIR)/$$test_name; \
		echo ""; \
		echo "编译 $$test_name..."; \
		if [ "$$test_name" = "test_status_line" ]; then \
			$(CC) $(CFLAGS) $$test_src $(filter-out $(SRC_DIR)/main.c,$(SRCS)) -o $$test_bin $(LDFLAGS) || { failed=1; continue; }; \
		else \
			$(CC) $(CFLAGS) -DUNIT_TEST $$test_src $(filter-out $(SRC_DIR)/main.c,$(SRCS)) -o $$test_bin $(LDFLAGS) || { failed=1; continue; }; \
		fi; \
		echo "运行 $$test_name..."; \
		$$test_bin || { failed=1; }; \
		rm -f $$test_bin; \
	done; \
	rm -rf $(TEST_BUILD_DIR); \
	echo ""; \
	echo "========================================"; \
	if [ $$failed -eq 0 ]; then \
		echo "所有测试通过！"; \
	else \
		echo "测试失败！"; \
		exit 1; \
	fi
	echo "========================================"

# 单独运行肉眼观察测试（不删除产物，便于调试）
view-test: $(TEST_BUILD_DIR)/test_status_line
	$(TEST_BUILD_DIR)/test_status_line | less -R

$(TEST_BUILD_DIR)/test_status_line: $(TEST_DIR)/test_status_line.c $(filter-out $(SRC_DIR)/main.c,$(SRCS))
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# ==================== 清理目标 ====================
.PHONY: clean distclean
clean:
	rm -rf $(TEST_BUILD_DIR)
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
	@echo "  make test         - 编译并运行所有测试（运行后自动删除产物）"
	@echo "  make clean        - 清理编译产物"
	@echo "  make distclean    - 清理所有生成文件"
	@echo "  make help         - 显示此帮助信息"
	@echo ""
	@echo "示例:"
	@echo "  make              # 编译到 bin/resource_usage"
	@echo "  make test         # 运行所有测试"
	@echo "  make clean        # 清理"
