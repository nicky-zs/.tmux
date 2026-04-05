#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "resource-usage.h"

#define CPU_WARNING_THRESHOLD 0.60
#define CPU_CRITICAL_THRESHOLD 0.85

static int narrow = 0;

static inline void exit_on_error(const char *msg) {
	errno ? perror(msg) : fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

/* ==================== 显示格式化 - 纯函数，可测试 ==================== */

const char *get_color_for_rate(double rate) {
	if (rate >= CPU_CRITICAL_THRESHOLD) {
		return "#[bg=colour1,fg=colour255]";
	}
	if (rate >= CPU_WARNING_THRESHOLD) {
		return "#[bg=colour3,fg=colour255]";
	}
	return "#[bg=colour10,fg=colour255]";  /* 正常时绿色背景 */
}

static const char *color(double rate) {
	if (rate == 0) {
		return "#[bg=colour10,fg=colour255]";  /* 重置为绿色背景 */
	}
	return get_color_for_rate(rate);
}

void display_cpu(resource_usage *cpu, const char *name, int narrow_mode) {
	if (!cpu) { return; }
	if (narrow_mode) {
		/* 空格在颜色外：只有内容飘红 */
		printf("%s%3.1f%%%s |", color(cpu->rate), 100.0 * cpu->rate, color(0));
	} else {
		/* emoji 跟着数值一起高亮，空格在绿色背景内 */
		printf("%s %s%s%3.1f%%%s", color(0), color(cpu->rate), name, 100.0 * cpu->rate, color(0));
	}
}

static const char units[] = "KMGTPEZY";

char new_unit(unsigned long long kb, double *new_size) {
	double size = kb;
	const char *p = units;
	while (size >= 1000) {
		size /= 1024;
		if (!*++p) {
			exit_on_error("memory too large");
		}
	}
	*new_size = size;
	return *p;
}

void display_mem(resource_usage *mem, int narrow_mode) {
	if (!mem || mem->total == 0) { return; }

	double in_use, total;
	double rate = ((double) mem->in_use) / mem->total;
	double new_size_double;
	char unit;

	unit = new_unit(mem->total, &new_size_double);
	total = new_size_double;
	in_use = rate * total;

	if (narrow_mode) {
		/* 空格在颜色外：只有内容飘红 */
		printf("%s%.1f/%.1f[%c]%s ", color(rate), in_use, total, unit, color(0));
	} else {
		/* 前导空格保持默认背景，只有数值飘红 */
		printf("%s %s%.1f/%.1f[%c]%s", color(0), color(rate), in_use, total, unit, color(0));
	}
}

/* ==================== 显示逻辑的生产环境封装 ==================== */

static void display_all(resource_usage *cpu, resource_usage *max_core, int cores,
                        resource_usage *mem, int narrow_mode) {
	display_cpu(cpu, "📊", narrow_mode);
	if (cores > 1) {
		display_cpu(max_core, "📈", narrow_mode);
	}
	display_mem(mem, narrow_mode);
	printf("\n");
	fflush(stdout);
}

/* ==================== 生产环境入口 ==================== */

int resource_usage_main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <tmux_socket_path> [narrow]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Initialize temp file path based on tmux socket
	init_cpu_tempfile(argv[1]);

	if (argc > 2 && strcmp(argv[2], "narrow") == 0) {
		narrow = 1;
	}

	resource_usage cpu;
	resource_usage max_core;
	int cores;
	if (cpu_usage(&cpu, &max_core, &cores) != 0) {
		exit_on_error("failed to get cpu usage");
	}

	resource_usage mem;
	if (mem_usage(&mem) != 0) {
		exit_on_error("failed to get memory usage");
	}

	display_all(&cpu, &max_core, cores, &mem, narrow);
	return 0;
}
