/*
 * 显示格式化逻辑的单元测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "../src/resource-usage.h"
#include "test_harness.h"

/* 声明被测试的函数 */
extern const char *get_color_for_rate(double rate);
extern void display_cpu(resource_usage *cpu, const char *name, int narrow_mode);
extern char new_unit(unsigned long long kb, double *new_size);
extern void display_mem(resource_usage *mem, int narrow_mode);

/* ==================== 颜色选择测试 ==================== */

static void test_color_normal(void) {
	/* 正常负载 (< 60%) 应该是绿色 (colour10) */
	const char *color = get_color_for_rate(0.0);
	ASSERT_STR_EQ("#[bg=colour10,fg=colour255]", color);

	color = get_color_for_rate(0.30);
	ASSERT_STR_EQ("#[bg=colour10,fg=colour255]", color);

	color = get_color_for_rate(0.59);
	ASSERT_STR_EQ("#[bg=colour10,fg=colour255]", color);
}

static void test_color_warning(void) {
	/* 警告负载 (60% - 85%) 应该是黄色 (colour3) */
	const char *color = get_color_for_rate(0.60);
	ASSERT_STR_EQ("#[bg=colour3,fg=colour255]", color);

	color = get_color_for_rate(0.70);
	ASSERT_STR_EQ("#[bg=colour3,fg=colour255]", color);

	color = get_color_for_rate(0.84);
	ASSERT_STR_EQ("#[bg=colour3,fg=colour255]", color);
}

static void test_color_critical(void) {
	/* 临界负载 (> 85%) 应该是红色 (colour1) */
	const char *color = get_color_for_rate(0.851);
	ASSERT_STR_EQ("#[bg=colour1,fg=colour255]", color);

	color = get_color_for_rate(0.90);
	ASSERT_STR_EQ("#[bg=colour1,fg=colour255]", color);

	color = get_color_for_rate(1.0);
	ASSERT_STR_EQ("#[bg=colour1,fg=colour255]", color);
}

/* ==================== display_cpu 测试 ==================== */

static void test_display_cpu_wide_mode(void) {
	resource_usage cpu = { .in_use = 50, .total = 100, .rate = 0.5 };

	/* 捕获输出 */
	char buffer[256];
	FILE *old_stdout = stdout;
	FILE *temp = fopen("/tmp/test_display_output", "w");
	ASSERT_TRUE(temp != NULL);
	stdout = temp;

	display_cpu(&cpu, "📊", 0);  /* 宽屏模式 */

	fflush(temp);
	stdout = old_stdout;
	fclose(temp);

	/* 读取输出 */
	temp = fopen("/tmp/test_display_output", "r");
	ASSERT_TRUE(temp != NULL);
	ASSERT_TRUE(fgets(buffer, sizeof(buffer), temp) != NULL);
	fclose(temp);
	unlink("/tmp/test_display_output");

	/* 验证输出包含预期内容 */
	ASSERT_TRUE(strstr(buffer, "📊") != NULL);
	ASSERT_TRUE(strstr(buffer, "50.0%") != NULL);
	ASSERT_TRUE(strstr(buffer, "colour10") != NULL);  /* 50% 应该是绿色 */
}

static void test_display_cpu_narrow_mode(void) {
	resource_usage cpu = { .in_use = 70, .total = 100, .rate = 0.7 };

	char buffer[256];
	FILE *old_stdout = stdout;
	FILE *temp = fopen("/tmp/test_display_output", "w");
	ASSERT_TRUE(temp != NULL);
	stdout = temp;

	display_cpu(&cpu, "📊", 1);  /* 窄屏模式 */

	fflush(temp);
	stdout = old_stdout;
	fclose(temp);

	temp = fopen("/tmp/test_display_output", "r");
	ASSERT_TRUE(temp != NULL);
	ASSERT_TRUE(fgets(buffer, sizeof(buffer), temp) != NULL);
	fclose(temp);
	unlink("/tmp/test_display_output");

	/* 窄屏模式不应该包含 emoji 名称 */
	ASSERT_TRUE(strstr(buffer, "70.0%") != NULL);
	ASSERT_TRUE(strstr(buffer, "colour3") != NULL);  /* 70% 应该是黄色 */
}

static void test_display_cpu_null_arg(void) {
	/* NULL 参数应该安全返回，不崩溃 */
	display_cpu(NULL, "📊", 0);
	display_cpu(NULL, "📊", 1);
}

/* ==================== new_unit 测试 ==================== */

static void test_new_unit_kilobytes(void) {
	double size;
	char unit = new_unit(500, &size);
	ASSERT_EQ('K', unit);
	ASSERT_DOUBLE_EQ(500.0, size, 0.01);
}

static void test_new_unit_megabytes(void) {
	double size;
	char unit = new_unit(1024, &size);
	ASSERT_EQ('M', unit);
	ASSERT_DOUBLE_EQ(1.0, size, 0.01);
}

static void test_new_unit_gigabytes(void) {
	double size;
	char unit = new_unit(1048576, &size);  /* 1024 * 1024 */
	ASSERT_EQ('G', unit);
	ASSERT_DOUBLE_EQ(1.0, size, 0.01);
}

static void test_new_unit_terabytes(void) {
	double size;
	char unit = new_unit(1073741824ULL, &size);  /* 1024^3 */
	ASSERT_EQ('T', unit);
	ASSERT_DOUBLE_EQ(1.0, size, 0.01);
}

static void test_new_unit_petabytes(void) {
	double size;
	char unit = new_unit(1099511627776ULL, &size);  /* 1024^4 */
	ASSERT_EQ('P', unit);
	ASSERT_DOUBLE_EQ(1.0, size, 0.01);
}

/* ==================== display_mem 测试 ==================== */

static void test_display_mem_wide_mode(void) {
	/* 16GB 总内存，使用 4GB */
	resource_usage mem = {
		.in_use = 4194304,  /* 4GB in KB */
		.total = 16777216,  /* 16GB in KB */
		.rate = 0.25
	};

	char buffer[512];
	FILE *old_stdout = stdout;
	FILE *temp = fopen("/tmp/test_display_output", "w");
	ASSERT_TRUE(temp != NULL);
	stdout = temp;

	display_mem(&mem, 0);  /* 宽屏模式 */

	fflush(temp);
	stdout = old_stdout;
	fclose(temp);

	temp = fopen("/tmp/test_display_output", "r");
	ASSERT_TRUE(temp != NULL);
	ASSERT_TRUE(fgets(buffer, sizeof(buffer), temp) != NULL);
	fclose(temp);
	unlink("/tmp/test_display_output");

	/* 验证输出包含预期内容 */
	ASSERT_TRUE(strstr(buffer, "4.0/16.0") != NULL);
	ASSERT_TRUE(strstr(buffer, "[G]") != NULL);
	ASSERT_TRUE(strstr(buffer, "colour10") != NULL);  /* 25% 应该是绿色 */
}

static void test_display_mem_narrow_mode(void) {
	resource_usage mem = {
		.in_use = 2097152,  /* 2GB in KB */
		.total = 8388608,   /* 8GB in KB */
		.rate = 0.25
	};

	char buffer[256];
	FILE *old_stdout = stdout;
	FILE *temp = fopen("/tmp/test_display_output", "w");
	ASSERT_TRUE(temp != NULL);
	stdout = temp;

	display_mem(&mem, 1);  /* 窄屏模式 */

	fflush(temp);
	stdout = old_stdout;
	fclose(temp);

	temp = fopen("/tmp/test_display_output", "r");
	ASSERT_TRUE(temp != NULL);
	ASSERT_TRUE(fgets(buffer, sizeof(buffer), temp) != NULL);
	fclose(temp);
	unlink("/tmp/test_display_output");

	ASSERT_TRUE(strstr(buffer, "2.0/8.0") != NULL);
	ASSERT_TRUE(strstr(buffer, "[G]") != NULL);
}

static void test_display_mem_null_arg(void) {
	/* NULL 参数应该安全返回 */
	display_mem(NULL, 0);
	display_mem(NULL, 1);
}

static void test_display_mem_zero_total(void) {
	/* total = 0 时应该安全返回 */
	resource_usage mem = { .in_use = 0, .total = 0, .rate = 0.0 };
	display_mem(&mem, 0);
	display_mem(&mem, 1);
}

/* ==================== 主函数 ==================== */

int main(void) {
	fprintf(stderr, "=== 显示格式化测试 ===\n\n");

	fprintf(stderr, "--- 颜色选择测试 ---\n");
	RUN_TEST(test_color_normal);
	RUN_TEST(test_color_warning);
	RUN_TEST(test_color_critical);

	fprintf(stderr, "\n--- display_cpu 测试 ---\n");
	RUN_TEST(test_display_cpu_wide_mode);
	RUN_TEST(test_display_cpu_narrow_mode);
	RUN_TEST(test_display_cpu_null_arg);

	fprintf(stderr, "\n--- new_unit 测试 ---\n");
	RUN_TEST(test_new_unit_kilobytes);
	RUN_TEST(test_new_unit_megabytes);
	RUN_TEST(test_new_unit_gigabytes);
	RUN_TEST(test_new_unit_terabytes);
	RUN_TEST(test_new_unit_petabytes);

	fprintf(stderr, "\n--- display_mem 测试 ---\n");
	RUN_TEST(test_display_mem_wide_mode);
	RUN_TEST(test_display_mem_narrow_mode);
	RUN_TEST(test_display_mem_null_arg);
	RUN_TEST(test_display_mem_zero_total);

	PRINT_TEST_RESULTS();
	return 0;
}
