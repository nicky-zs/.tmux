# tmux resource usage 测试 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lm

# 源文件
SRC_DIR = src
TEST_DIR = tests

# 生产代码源文件
SRCS = $(SRC_DIR)/cpu.c $(SRC_DIR)/mem.c $(SRC_DIR)/resource-usage.c $(SRC_DIR)/main.c

# 测试源文件
TEST_CPU_SRC = $(TEST_DIR)/test_cpu.c
TEST_MEM_SRC = $(TEST_DIR)/test_mem.c
TEST_DISPLAY_SRC = $(TEST_DIR)/test_display.c

# 测试可执行文件
TEST_CPU = test_cpu_runner
TEST_MEM = test_mem_runner
TEST_DISPLAY = test_display_runner

# 生产可执行文件
BIN_DIR = bin
RESOURCE_USAGE = $(BIN_DIR)/resource_usage

.PHONY: all tests clean test-cpu test-mem test-display run-tests

all: $(RESOURCE_USAGE)

tests: $(TEST_CPU) $(TEST_MEM) $(TEST_DISPLAY)

# 生产代码编译
$(RESOURCE_USAGE): $(SRCS)
	@test -d $(BIN_DIR) || mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

# 测试编译（不包含 main.c）
$(TEST_CPU): $(TEST_CPU_SRC) $(SRC_DIR)/cpu.c $(SRC_DIR)/mem.c $(SRC_DIR)/resource-usage.c
	$(CC) $(CFLAGS) -DUNIT_TEST $(TEST_CPU_SRC) $(SRC_DIR)/cpu.c $(SRC_DIR)/mem.c $(SRC_DIR)/resource-usage.c -o $@ $(LDFLAGS)

$(TEST_MEM): $(TEST_MEM_SRC) $(SRC_DIR)/cpu.c $(SRC_DIR)/mem.c $(SRC_DIR)/resource-usage.c
	$(CC) $(CFLAGS) -DUNIT_TEST $(TEST_MEM_SRC) $(SRC_DIR)/cpu.c $(SRC_DIR)/mem.c $(SRC_DIR)/resource-usage.c -o $@ $(LDFLAGS)

$(TEST_DISPLAY): $(TEST_DISPLAY_SRC) $(SRC_DIR)/cpu.c $(SRC_DIR)/mem.c $(SRC_DIR)/resource-usage.c
	$(CC) $(CFLAGS) -DUNIT_TEST $(TEST_DISPLAY_SRC) $(SRC_DIR)/cpu.c $(SRC_DIR)/mem.c $(SRC_DIR)/resource-usage.c -o $@ $(LDFLAGS)

# 运行所有测试
run-tests: tests
	@echo "Running CPU tests..."
	@$(TEST_CPU)
	@echo ""
	@echo "Running Memory tests..."
	@$(TEST_MEM)
	@echo ""
	@echo "Running Display tests..."
	@$(TEST_DISPLAY)

clean:
	rm -f $(TEST_CPU) $(TEST_MEM) $(TEST_DISPLAY)
	rm -f $(RESOURCE_USAGE)
	rm -f /tmp/test_display_output
	rm -f /tmp/test_resource_*
