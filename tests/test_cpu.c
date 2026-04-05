/*
 * CPU 使用率计算逻辑的单元测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "../src/resource-usage.h"
#include "test_harness.h"

/* ==================== record_list 测试 ==================== */

static void test_record_list_init(void) {
	record_list list;
	int ret = record_list_init(&list, 128);
	ASSERT_EQ(0, ret);
	ASSERT_EQ(0, list.len);
	ASSERT_EQ(128, list.cap);
	ASSERT_TRUE(list.elem != NULL);
	record_list_destroy(&list);
}

static void test_record_list_init_invalid_args(void) {
	record_list list;
	int ret = record_list_init(NULL, 128);
	ASSERT_EQ(-1, ret);

	ret = record_list_init(&list, 0);
	ASSERT_EQ(-1, ret);
}

static void test_record_list_add_single(void) {
	record_list list;
	ASSERT_EQ(0, record_list_init(&list, 2));

	cpu_record rec = { .busy = 100, .total = 200 };
	ASSERT_EQ(0, record_list_add(&list, &rec));
	ASSERT_EQ(1, list.len);
	ASSERT_EQ(100, list.elem[0].busy);
	ASSERT_EQ(200, list.elem[0].total);

	record_list_destroy(&list);
}

static void test_record_list_add_multiple(void) {
	record_list list;
	ASSERT_EQ(0, record_list_init(&list, 2));

	cpu_record rec1 = { .busy = 100, .total = 200 };
	cpu_record rec2 = { .busy = 300, .total = 400 };
	cpu_record rec3 = { .busy = 500, .total = 600 };

	ASSERT_EQ(0, record_list_add(&list, &rec1));
	ASSERT_EQ(1, list.len);
	ASSERT_EQ(0, record_list_add(&list, &rec2));
	ASSERT_EQ(2, list.len);
	ASSERT_EQ(300, list.elem[1].busy);

	/* 触发扩容 */
	ASSERT_EQ(0, record_list_add(&list, &rec3));
	ASSERT_EQ(3, list.len);
	ASSERT_EQ(500, list.elem[2].busy);

	record_list_destroy(&list);
}

/* ==================== read_proc_stat_from_stream 测试 ==================== */

static void test_read_proc_stat_single_cpu(void) {
	const char *proc_stat =
		"cpu  100 50 200 1000 50 10 5\n"
		"cpu0 100 50 200 1000 50 10 5\n"
		"intr 1000\n"
		"ctxt 500\n";

	FILE *stream = open_string_stream(proc_stat);
	ASSERT_TRUE(stream != NULL);

	record_list list = { 0 };
	ASSERT_EQ(0, record_list_init(&list, 16));
	ASSERT_EQ(0, read_proc_stat_from_stream(stream, &list));

	/* 应该有 2 条记录：cpu (总体) + cpu0 */
	ASSERT_EQ(2, list.len);

	/* cpu 行：busy = 100+50+200+10+5 = 365, total = 100+50+200+1000+50+10+5 = 1415 */
	ASSERT_EQ(365, list.elem[0].busy);
	ASSERT_EQ(1415, list.elem[0].total);

	fclose(stream);
	record_list_destroy(&list);
}

static void test_read_proc_stat_multiple_cpus(void) {
	const char *proc_stat =
		"cpu  400 200 800 4000 200 40 20\n"
		"cpu0 100 50 200 1000 50 10 5\n"
		"cpu1 150 75 300 1500 75 15 7\n"
		"cpu2 100 50 200 1000 50 10 5\n"
		"cpu3 50 25 100 500 25 5 3\n"
		"intr 1000\n";

	FILE *stream = open_string_stream(proc_stat);
	ASSERT_TRUE(stream != NULL);

	record_list list = { 0 };
	ASSERT_EQ(0, record_list_init(&list, 16));
	ASSERT_EQ(0, read_proc_stat_from_stream(stream, &list));

	/* 应该有 5 条记录：cpu + cpu0 + cpu1 + cpu2 + cpu3 */
	ASSERT_EQ(5, list.len);

	fclose(stream);
	record_list_destroy(&list);
}

static void test_read_proc_stat_empty(void) {
	const char *proc_stat = "";

	FILE *stream = open_string_stream(proc_stat);
	ASSERT_TRUE(stream != NULL);

	record_list list = { 0 };
	ASSERT_EQ(0, record_list_init(&list, 16));

	/* 空文件应该返回 -1 */
	ASSERT_EQ(-1, read_proc_stat_from_stream(stream, &list));

	fclose(stream);
	record_list_destroy(&list);
}

static void test_read_proc_stat_no_cpu_lines(void) {
	const char *proc_stat =
		"intr 1000\n"
		"ctxt 500\n"
		"btime 1234567890\n";

	FILE *stream = open_string_stream(proc_stat);
	ASSERT_TRUE(stream != NULL);

	record_list list = { 0 };
	ASSERT_EQ(0, record_list_init(&list, 16));

	/* 没有 cpu 行应该返回 -1 */
	ASSERT_EQ(-1, read_proc_stat_from_stream(stream, &list));

	fclose(stream);
	record_list_destroy(&list);
}

/* ==================== compute_cpu_usage 测试 ==================== */

static void test_compute_cpu_usage_basic(void) {
	record_list prev, current;
	ASSERT_EQ(0, record_list_init(&prev, 16));
	ASSERT_EQ(0, record_list_init(&current, 16));

	/* 总体 CPU: prev busy=100, total=1000; current busy=200, total=2000 */
	/* delta: busy=100, total=1000, rate=0.1 */
	cpu_record prev_cpu = { .busy = 100, .total = 1000 };
	cpu_record curr_cpu = { .busy = 200, .total = 2000 };
	ASSERT_EQ(0, record_list_add(&prev, &prev_cpu));
	ASSERT_EQ(0, record_list_add(&current, &curr_cpu));

	resource_usage cpu, max_core;
	int cores;

	ASSERT_EQ(0, compute_cpu_usage(&prev, &current, &cpu, &max_core, &cores));
	ASSERT_EQ(0, cores);  /* 只有总体，没有单核 */
	ASSERT_EQ(100, cpu.in_use);
	ASSERT_EQ(1000, cpu.total);
	ASSERT_DOUBLE_EQ(0.1, cpu.rate, 0.001);

	record_list_destroy(&prev);
	record_list_destroy(&current);
}

static void test_compute_cpu_usage_with_cores(void) {
	record_list prev, current;
	ASSERT_EQ(0, record_list_init(&prev, 16));
	ASSERT_EQ(0, record_list_init(&current, 16));

	/* cpu (总体) */
	ASSERT_EQ(0, record_list_add(&prev, &(cpu_record){ .busy = 1000, .total = 10000 }));
	ASSERT_EQ(0, record_list_add(&current, &(cpu_record){ .busy = 2000, .total = 20000 }));

	/* cpu0 */
	ASSERT_EQ(0, record_list_add(&prev, &(cpu_record){ .busy = 200, .total = 2000 }));
	ASSERT_EQ(0, record_list_add(&current, &(cpu_record){ .busy = 600, .total = 4000 }));

	/* cpu1 */
	ASSERT_EQ(0, record_list_add(&prev, &(cpu_record){ .busy = 300, .total = 3000 }));
	ASSERT_EQ(0, record_list_add(&current, &(cpu_record){ .busy = 450, .total = 4500 }));

	resource_usage cpu, max_core;
	int cores;

	ASSERT_EQ(0, compute_cpu_usage(&prev, &current, &cpu, &max_core, &cores));
	ASSERT_EQ(2, cores);  /* 2 个单核 */

	/* 总体：delta busy=1000, total=10000, rate=0.1 */
	ASSERT_EQ(1000, cpu.in_use);
	ASSERT_EQ(10000, cpu.total);
	ASSERT_DOUBLE_EQ(0.1, cpu.rate, 0.001);

	/* cpu0: delta busy=400, total=2000, rate=0.2 */
	/* cpu1: delta busy=150, total=1500, rate=0.1 */
	/* max_core 应该是 cpu0 (rate=0.2) */
	ASSERT_DOUBLE_EQ(0.2, max_core.rate, 0.001);
	ASSERT_EQ(400, max_core.in_use);
	ASSERT_EQ(2000, max_core.total);

	record_list_destroy(&prev);
	record_list_destroy(&current);
}

static void test_compute_cpu_usage_invalid_args(void) {
	record_list prev, current;
	ASSERT_EQ(0, record_list_init(&prev, 16));
	ASSERT_EQ(0, record_list_init(&current, 16));

	resource_usage cpu, max_core;
	int cores;

	/* NULL 参数 */
	ASSERT_EQ(-1, compute_cpu_usage(NULL, &current, &cpu, &max_core, &cores));
	ASSERT_EQ(-1, compute_cpu_usage(&prev, NULL, &cpu, &max_core, &cores));
	ASSERT_EQ(-1, compute_cpu_usage(&prev, &current, NULL, &max_core, &cores));
	ASSERT_EQ(-1, compute_cpu_usage(&prev, &current, &cpu, NULL, &cores));
	ASSERT_EQ(-1, compute_cpu_usage(&prev, &current, &cpu, &max_core, NULL));

	record_list_destroy(&prev);
	record_list_destroy(&current);
}

static void test_compute_cpu_usage_empty_current(void) {
	record_list prev, current;
	ASSERT_EQ(0, record_list_init(&prev, 16));
	ASSERT_EQ(0, record_list_init(&current, 16));

	resource_usage cpu, max_core;
	int cores;

	/* current 为空 */
	ASSERT_EQ(-1, compute_cpu_usage(&prev, &current, &cpu, &max_core, &cores));

	record_list_destroy(&prev);
	record_list_destroy(&current);
}

static void test_compute_cpu_usage_length_mismatch(void) {
	record_list prev, current;
	ASSERT_EQ(0, record_list_init(&prev, 16));
	ASSERT_EQ(0, record_list_init(&current, 16));

	ASSERT_EQ(0, record_list_add(&prev, &(cpu_record){ .busy = 100, .total = 1000 }));
	ASSERT_EQ(0, record_list_add(&current, &(cpu_record){ .busy = 200, .total = 2000 }));
	ASSERT_EQ(0, record_list_add(&current, &(cpu_record){ .busy = 300, .total = 3000 }));

	resource_usage cpu, max_core;
	int cores;

	/* 长度不匹配应该返回 -2 */
	ASSERT_EQ(-2, compute_cpu_usage(&prev, &current, &cpu, &max_core, &cores));

	record_list_destroy(&prev);
	record_list_destroy(&current);
}

static void test_compute_cpu_usage_zero_delta_total(void) {
	record_list prev, current;
	ASSERT_EQ(0, record_list_init(&prev, 16));
	ASSERT_EQ(0, record_list_init(&current, 16));

	/* delta total = 0 的情况 */
	ASSERT_EQ(0, record_list_add(&prev, &(cpu_record){ .busy = 100, .total = 1000 }));
	ASSERT_EQ(0, record_list_add(&current, &(cpu_record){ .busy = 100, .total = 1000 }));

	resource_usage cpu, max_core;
	int cores;

	ASSERT_EQ(0, compute_cpu_usage(&prev, &current, &cpu, &max_core, &cores));
	ASSERT_EQ(0, cpu.in_use);
	ASSERT_EQ(0, cpu.total);
	ASSERT_DOUBLE_EQ(0.0, cpu.rate, 0.001);

	record_list_destroy(&prev);
	record_list_destroy(&current);
}

/* ==================== record_list Load/Save 测试 ==================== */

static void test_record_list_load_save_roundtrip(void) {
	char *path = create_temp_file_with_content(NULL, 0);
	ASSERT_TRUE(path != NULL);

	record_list saved = { 0 };
	ASSERT_EQ(0, record_list_init(&saved, 16));
	ASSERT_EQ(0, record_list_add(&saved, &(cpu_record){ .busy = 100, .total = 1000 }));
	ASSERT_EQ(0, record_list_add(&saved, &(cpu_record){ .busy = 200, .total = 2000 }));
	ASSERT_EQ(0, record_list_add(&saved, &(cpu_record){ .busy = 300, .total = 3000 }));

	ASSERT_EQ(0, record_list_dump(&saved, path));

	record_list loaded = { 0 };
	ASSERT_EQ(0, record_list_load(&loaded, path));

	ASSERT_EQ(3, loaded.len);
	ASSERT_EQ(100, loaded.elem[0].busy);
	ASSERT_EQ(1000, loaded.elem[0].total);
	ASSERT_EQ(200, loaded.elem[1].busy);
	ASSERT_EQ(2000, loaded.elem[1].total);
	ASSERT_EQ(300, loaded.elem[2].busy);
	ASSERT_EQ(3000, loaded.elem[2].total);

	record_list_destroy(&saved);
	record_list_destroy(&loaded);
	remove_temp_file(path);
}

static void test_record_list_load_nonexistent_file(void) {
	record_list list = { 0 };
	ASSERT_EQ(-1, record_list_load(&list, "/nonexistent/path/file_12345.data"));
}

static void test_record_list_load_empty_file(void) {
	char *path = create_temp_file_with_content(NULL, 0);
	ASSERT_TRUE(path != NULL);

	record_list list = { 0 };
	ASSERT_EQ(-1, record_list_load(&list, path));

	remove_temp_file(path);
}

static void test_record_list_load_corrupted_length(void) {
	/* 写入一个巨大的长度值，测试合理性检查 */
	char *path = create_temp_file_with_content(NULL, 0);
	ASSERT_TRUE(path != NULL);

	FILE *f = fopen(path, "wb");
	ASSERT_TRUE(f != NULL);

	size_t huge_len = 9999999;  /* 超过 1024 的限制 */
	fwrite(&huge_len, sizeof(huge_len), 1, f);
	fclose(f);

	record_list list = { 0 };
	ASSERT_EQ(-1, record_list_load(&list, path));

	remove_temp_file(path);
}

/* ==================== 主函数 ==================== */

int main(void) {
	fprintf(stderr, "=== CPU 测试 ===\n\n");

	/* record_list 测试 */
	fprintf(stderr, "--- record_list 测试 ---\n");
	RUN_TEST(test_record_list_init);
	RUN_TEST(test_record_list_init_invalid_args);
	RUN_TEST(test_record_list_add_single);
	RUN_TEST(test_record_list_add_multiple);

	/* read_proc_stat 测试 */
	fprintf(stderr, "\n--- read_proc_stat_from_stream 测试 ---\n");
	RUN_TEST(test_read_proc_stat_single_cpu);
	RUN_TEST(test_read_proc_stat_multiple_cpus);
	RUN_TEST(test_read_proc_stat_empty);
	RUN_TEST(test_read_proc_stat_no_cpu_lines);

	/* compute_cpu_usage 测试 */
	fprintf(stderr, "\n--- compute_cpu_usage 测试 ---\n");
	RUN_TEST(test_compute_cpu_usage_basic);
	RUN_TEST(test_compute_cpu_usage_with_cores);
	RUN_TEST(test_compute_cpu_usage_invalid_args);
	RUN_TEST(test_compute_cpu_usage_empty_current);
	RUN_TEST(test_compute_cpu_usage_length_mismatch);
	RUN_TEST(test_compute_cpu_usage_zero_delta_total);

	/* Load/Save 测试 */
	fprintf(stderr, "\n--- record_list Load/Save 测试 ---\n");
	RUN_TEST(test_record_list_load_save_roundtrip);
	RUN_TEST(test_record_list_load_nonexistent_file);
	RUN_TEST(test_record_list_load_empty_file);
	RUN_TEST(test_record_list_load_corrupted_length);

	PRINT_TEST_RESULTS();
	return 0;
}
