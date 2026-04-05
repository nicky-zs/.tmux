# tmux resource usage - Makefile
# 标准 GNU Makefile 规范，支持安装、测试、清理等目标

# ==================== 编译器与标志 ====================
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -g
LDFLAGS := -lm

# 生产环境优化标志（可选：make release）
ifdef RELEASE
CFLAGS := -O3 -DNDEBUG -Wall -Wextra
endif

# ==================== 目录结构 ====================
PREFIX      ?= /usr/local
BINDIR      ?= $(PREFIX)/bin
SRC_DIR     := src
TEST_DIR    := tests
BUILD_DIR   := bin

# ==================== 源文件与目标 ====================
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:.c=.o)

# 生产目标
RESOURCE_USAGE := $(BUILD_DIR)/resource_usage

# 测试目标
TEST_SRCS  := $(TEST_DIR)/test_cpu.c $(TEST_DIR)/test_mem.c $(TEST_DIR)/test_display.c
TEST_BINS  := $(TEST_DIR)/test_cpu_runner \
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
.PHONY: test tests
test: tests
tests: $(TEST_BINS)

# 测试编译（排除 main.c，使用测试源文件）
$(TEST_DIR)/test_cpu_runner: $(TEST_DIR)/test_cpu.c $(filter-out $(SRC_DIR)/main.c,$(SRCS))
	$(CC) $(CFLAGS) -DUNIT_TEST $^ -o $@ $(LDFLAGS)

$(TEST_DIR)/test_mem_runner: $(TEST_DIR)/test_mem.c $(filter-out $(SRC_DIR)/main.c,$(SRCS))
	$(CC) $(CFLAGS) -DUNIT_TEST $^ -o $@ $(LDFLAGS)

$(TEST_DIR)/test_display_runner: $(TEST_DIR)/test_display.c $(filter-out $(SRC_DIR)/main.c,$(SRCS))
	$(CC) $(CFLAGS) -DUNIT_TEST $^ -o $@ $(LDFLAGS)

# 运行测试
.PHONY: run-tests check
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

# ==================== 安装目标 ====================
.PHONY: install
install: $(RESOURCE_USAGE)
	@echo "安装到 $(DESTDIR)$(BINDIR)..."
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(RESOURCE_USAGE) $(DESTDIR)$(BINDIR)/resource_usage
	@echo "安装完成。"

# ==================== 卸载目标 ====================
.PHONY: uninstall
uninstall:
	@echo "从 $(DESTDIR)$(BINDIR) 卸载..."
	rm -f $(DESTDIR)$(BINDIR)/resource_usage
	@echo "卸载完成。"

# ==================== 清理目标 ====================
.PHONY: clean distclean
clean:
	rm -f $(OBJS)
	rm -f $(RESOURCE_USAGE)
	rm -f $(TEST_BINS)
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
	@echo "  make / make all   - 编译生产版本 (默认)"
	@echo "  make release      - 编译优化版本 (O3 优化)"
	@echo "  make test         - 编译所有测试"
	@echo "  make run-tests    - 运行所有测试"
	@echo "  make install      - 安装到 $(DESTDIR)$(BINDIR)"
	@echo "  make uninstall    - 从 $(DESTDIR)$(BINDIR) 卸载"
	@echo "  make clean        - 清理编译产物"
	@echo "  make distclean    - 清理所有生成文件"
	@echo "  make help         - 显示此帮助信息"
	@echo ""
	@echo "变量:"
	@echo "  PREFIX=$(PREFIX)      - 安装前缀 (默认：/usr/local)"
	@echo "  DESTDIR=          - 安装根目录 (用于打包)"
	@echo ""
	@echo "示例:"
	@echo "  make                              # 编译"
	@echo "  make test && make run-tests       # 运行测试"
	@echo "  sudo make install                 # 安装到 /usr/local/bin"
	@echo "  make PREFIX=/opt/tmux install     # 安装到 /opt/tmux/bin"
