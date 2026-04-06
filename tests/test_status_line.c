/*
 * test_status_line.c - 穷举所有显示状态组合
 *
 * 测试组合：[宽屏，窄屏] × CPU[低，中，高] × 单核 [低，中，高] × 内存 [低，中，高]
 * 共 2 × 3 × 3 × 3 = 54 种组合
 *
 * 输出格式：一行一个效果，直接带颜色显示，包含时间栏
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/resource-usage.h"

/* 率值标签 */
static const char *rate_label(double rate) {
	if (rate < 0.5) return "L";  /* Low */
	if (rate < 0.75) return "M"; /* Medium */
	return "H";                  /* High */
}

/* 打印场景标签（紧凑型） */
static void print_label(int narrow, double cpu_rate, double max_core_rate,
                        double mem_rate) {
	const char *mode = narrow ? "窄" : "宽";
	printf("\033[2m%s|CPU%s|核%s|Mem%s|\033[0m ",
	       mode,
	       rate_label(cpu_rate),
	       max_core_rate > 0 ? rate_label(max_core_rate) : "-",
	       rate_label(mem_rate));
}

/* 单行打印一个场景的效果 - 使用统一函数 format_full_status_line */
static void print_single_line(int narrow, double cpu_rate, double max_core_rate,
                              int cores, double mem_rate, unsigned long long mem_total_kb) {
	display_state state = {
		.narrow_mode = narrow,
		.cpu_rate = cpu_rate,
		.max_core_rate = max_core_rate,
		.core_count = cores,
		.mem_rate = mem_rate,
		.mem_total_kb = mem_total_kb,
	};

	char buffer[1024];
	/* 使用统一函数，确保测试和生产走同一段代码 */
	format_full_status_line(&state, !narrow, buffer, sizeof(buffer));

	/* 转换为 ANSI 并打印 */
	char ansi_buffer[2048];
	render_tmux_codes_to_ansi(buffer, ansi_buffer, sizeof(ansi_buffer));
	printf("%s\033[0m\n", ansi_buffer);
}

/* 穷举所有组合 - 紧凑输出 */
static void run_all_combinations_compact(void) {
	double cpu_rates[] = {0.15, 0.50, 0.90};    /* 低，中，高 */
	double core_rates[] = {0.10, 0.55, 0.95};   /* 低，中，高 */
	double mem_rates[] = {0.20, 0.50, 0.85};    /* 低，中，高 */
	unsigned long long mem_total = 16 * 1024 * 1024;  /* 16GB KB */

	/* 先打印所有宽屏 */
	printf("\n\033[1m=== 宽屏模式 ===\033[0m\n\n");
	for (size_t ci = 0; ci < sizeof(cpu_rates)/sizeof(cpu_rates[0]); ci++) {
		for (size_t mi = 0; mi < sizeof(core_rates)/sizeof(core_rates[0]); mi++) {
			for (size_t mi2 = 0; mi2 < sizeof(mem_rates)/sizeof(mem_rates[0]); mi2++) {
				print_label(0, cpu_rates[ci], core_rates[mi], mem_rates[mi2]);
				print_single_line(0, cpu_rates[ci], core_rates[mi], 8, mem_rates[mi2], mem_total);
			}
		}
	}

	/* 再打印所有窄屏 */
	printf("\n\033[1m=== 窄屏模式 ===\033[0m\n\n");
	for (size_t ci = 0; ci < sizeof(cpu_rates)/sizeof(cpu_rates[0]); ci++) {
		for (size_t mi = 0; mi < sizeof(core_rates)/sizeof(core_rates[0]); mi++) {
			for (size_t mi2 = 0; mi2 < sizeof(mem_rates)/sizeof(mem_rates[0]); mi2++) {
				print_label(1, cpu_rates[ci], core_rates[mi], mem_rates[mi2]);
				print_single_line(1, cpu_rates[ci], core_rates[mi], 8, mem_rates[mi2], mem_total);
			}
		}
	}
}

/* 边界情况测试 */
static void run_edge_cases_compact(void) {
	unsigned long long mem_total = 16 * 1024 * 1024;

	printf("\033[1m=== 边界情况 ===\033[0m\n\n");

	/* 0% 利用率 - 三项都显示，单核也是 0% */
	printf("0%% 负载：");
	print_single_line(0, 0.0, 0.0, 8, 0.0, mem_total);

	/* 100% 利用率 - 三项都显示，单核也是 100% */
	printf("100%% 负载：");
	print_single_line(0, 1.0, 1.0, 8, 1.0, mem_total);

	/* 阈值边界 - 三项都显示，单核与整体同步 */
	printf("59%% (临界)：");
	print_single_line(0, 0.59, 0.59, 8, 0.59, mem_total);

	printf("60%% (警告)：");
	print_single_line(0, 0.60, 0.60, 8, 0.60, mem_total);

	printf("84%% (临界)：");
	print_single_line(0, 0.84, 0.84, 8, 0.84, mem_total);

	printf("85%% (严重)：");
	print_single_line(0, 0.85, 0.85, 8, 0.85, mem_total);

	printf("\n");
}

int main(void) {
	printf("\n");
	printf("================================================================================\n");
	printf("状态行显示测试 - 穷举所有组合 (紧凑模式)\n");
	printf("组合维度：[宽屏，窄屏] × CPU[低，中，高] × 单核 [低，中，高] × 内存 [低，中，高]\n");
	printf("================================================================================\n");

	/* 运行所有组合测试 */
	run_all_combinations_compact();

	/* 运行边界情况测试 */
	run_edge_cases_compact();

	printf("================================================================================\n");
	printf("测试完成\n");
	printf("================================================================================\n");

	return 0;
}
