/*
 * 内存使用率计算逻辑的单元测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/resource-usage.h"
#include "test_harness.h"

/* ==================== parse_meminfo 测试 ==================== */

static void test_parse_meminfo_basic(void) {
	const char *meminfo =
		"MemTotal:       16000000 kB\n"
		"MemFree:         8000000 kB\n"
		"MemAvailable:   12000000 kB\n"
		"Buffers:          500000 kB\n"
		"Cached:          3500000 kB\n"
		"SwapTotal:       8000000 kB\n"
		"SwapFree:        8000000 kB\n";

	FILE *stream = open_string_stream(meminfo);
	ASSERT_TRUE(stream != NULL);

	resource_usage mem;
	ASSERT_EQ(0, parse_meminfo(stream, &mem));

	/* MemTotal = 16000000 */
	ASSERT_EQ(16000000, mem.total);

	/* MemAvailable = 12000000, so in_use = 16000000 - 12000000 = 4000000 */
	ASSERT_EQ(4000000, mem.in_use);

	/* rate = 4000000 / 16000000 = 0.25 */
	ASSERT_DOUBLE_EQ(0.25, mem.rate, 0.001);

	fclose(stream);
}

static void test_parse_meminfo_without_memavail(void) {
	/* 模拟旧内核，没有 MemAvailable 字段 */
	const char *meminfo =
		"MemTotal:       16000000 kB\n"
		"MemFree:         8000000 kB\n"
		"Buffers:          500000 kB\n"
		"Cached:          3500000 kB\n";

	FILE *stream = open_string_stream(meminfo);
	ASSERT_TRUE(stream != NULL);

	resource_usage mem;
	ASSERT_EQ(0, parse_meminfo(stream, &mem));

	/* MemAvailable = MemFree + Buffers + Cached = 8000000 + 500000 + 3500000 = 12000000 */
	/* in_use = 16000000 - 12000000 = 4000000 */
	ASSERT_EQ(4000000, mem.in_use);
	ASSERT_DOUBLE_EQ(0.25, mem.rate, 0.001);

	fclose(stream);
}

static void test_parse_meminfo_minimal(void) {
	/* 只有必需字段 */
	const char *meminfo =
		"MemTotal:       1000000 kB\n"
		"MemFree:         500000 kB\n"
		"Buffers:          100000 kB\n"
		"Cached:           100000 kB\n";

	FILE *stream = open_string_stream(meminfo);
	ASSERT_TRUE(stream != NULL);

	resource_usage mem;
	ASSERT_EQ(0, parse_meminfo(stream, &mem));

	/* MemAvailable = 500000 + 100000 + 100000 = 700000 */
	/* in_use = 1000000 - 700000 = 300000 */
	ASSERT_EQ(300000, mem.in_use);
	ASSERT_DOUBLE_EQ(0.3, mem.rate, 0.001);

	fclose(stream);
}

static void test_parse_meminfo_missing_required_field(void) {
	/* 缺少 Buffers */
	const char *meminfo =
		"MemTotal:       1000000 kB\n"
		"MemFree:         500000 kB\n"
		"Cached:           100000 kB\n";

	FILE *stream = open_string_stream(meminfo);
	ASSERT_TRUE(stream != NULL);

	resource_usage mem;
	ASSERT_EQ(-1, parse_meminfo(stream, &mem));  /* 应该失败 */

	fclose(stream);
}

static void test_parse_meminfo_empty(void) {
	FILE *stream = open_string_stream("");
	ASSERT_TRUE(stream != NULL);

	resource_usage mem;
	ASSERT_EQ(-1, parse_meminfo(stream, &mem));  /* 应该失败 */

	fclose(stream);
}

static void test_parse_meminfo_null_args(void) {
	ASSERT_EQ(-1, parse_meminfo(NULL, NULL));

	resource_usage mem;
	ASSERT_EQ(-1, parse_meminfo(NULL, &mem));
}

static void test_parse_meminfo_memavail_exceeds_total(void) {
	/* MemAvailable > MemTotal 的异常情况 */
	const char *meminfo =
		"MemTotal:       1000000 kB\n"
		"MemFree:         800000 kB\n"
		"MemAvailable:   1500000 kB\n"  /* 异常：超过 Total */
		"Buffers:          100000 kB\n"
		"Cached:           100000 kB\n";

	FILE *stream = open_string_stream(meminfo);
	ASSERT_TRUE(stream != NULL);

	resource_usage mem;
	ASSERT_EQ(0, parse_meminfo(stream, &mem));

	/* 应该有边界检查，mem_avail 被限制为 mem_total */
	/* in_use = 1000000 - 1000000 = 0 */
	ASSERT_EQ(0, mem.in_use);
	ASSERT_DOUBLE_EQ(0.0, mem.rate, 0.001);

	fclose(stream);
}

static void test_parse_meminfo_zero_total(void) {
	/* MemTotal = 0 的边界情况 */
	const char *meminfo =
		"MemTotal:       0 kB\n"
		"MemFree:         0 kB\n"
		"MemAvailable:   0 kB\n"
		"Buffers:          0 kB\n"
		"Cached:           0 kB\n";

	FILE *stream = open_string_stream(meminfo);
	ASSERT_TRUE(stream != NULL);

	resource_usage mem;
	ASSERT_EQ(0, parse_meminfo(stream, &mem));

	ASSERT_EQ(0, mem.total);
	ASSERT_EQ(0, mem.in_use);
	ASSERT_DOUBLE_EQ(0.0, mem.rate, 0.001);  /* rate 应该是 0，避免除零 */

	fclose(stream);
}

/* ==================== mem_usage 生产函数测试 ==================== */

static void test_mem_usage_real_proc_meminfo(void) {
	/* 测试真实的生产环境函数 */
	resource_usage mem;
	ASSERT_EQ(0, mem_usage(&mem));

	/* 验证结果合理性 */
	ASSERT_TRUE(mem.total > 0);
	ASSERT_TRUE(mem.in_use <= mem.total);
	ASSERT_TRUE(mem.rate >= 0.0 && mem.rate <= 1.0);
}

static void test_mem_usage_null_arg(void) {
	ASSERT_EQ(-1, mem_usage(NULL));
}

/* ==================== 主函数 ==================== */

int main(void) {
	fprintf(stderr, "=== 内存测试 ===\n\n");

	fprintf(stderr, "--- parse_meminfo 测试 ---\n");
	RUN_TEST(test_parse_meminfo_basic);
	RUN_TEST(test_parse_meminfo_without_memavail);
	RUN_TEST(test_parse_meminfo_minimal);
	RUN_TEST(test_parse_meminfo_missing_required_field);
	RUN_TEST(test_parse_meminfo_empty);
	RUN_TEST(test_parse_meminfo_null_args);
	RUN_TEST(test_parse_meminfo_memavail_exceeds_total);
	RUN_TEST(test_parse_meminfo_zero_total);

	fprintf(stderr, "\n--- mem_usage 生产函数测试 ---\n");
	RUN_TEST(test_mem_usage_real_proc_meminfo);
	RUN_TEST(test_mem_usage_null_arg);

	PRINT_TEST_RESULTS();
	return 0;
}
