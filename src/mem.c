#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource-usage.h"

/* ==================== 内存信息解析 - 可测试 ==================== */

int parse_meminfo(FILE *stream, resource_usage *mem) {
	if (!mem || !stream) { return -1; }

	long long mem_total = -1, mem_free = -1, mem_avail = -1;
	long long mem_buffers = -1, mem_cached = -1;

	size_t line_size = 0;
	char *line = NULL;
	char *delim = " :";
	char *token, *saveptr;

	while (getline(&line, &line_size, stream) != -1) {
		token = strtok_r(line, delim, &saveptr);
		if (!token) continue;

		long long *target = NULL;

		if (starts_with(token, "MemTotal")) {
			target = &mem_total;
		} else if (starts_with(token, "MemFree")) {
			target = &mem_free;
		} else if (starts_with(token, "MemAvailable")) {
			target = &mem_avail;
		} else if (starts_with(token, "Buffers")) {
			target = &mem_buffers;
		} else if (starts_with(token, "Cached")) {
			target = &mem_cached;
		}

		if (target) {
			token = strtok_r(NULL, delim, &saveptr);
			if (token) {
				*target = strtoull(token, NULL, 10);
			}
		}
	}

	free(line);

	// 验证必需字段
	if (mem_total == -1 || mem_free == -1 || mem_buffers == -1 || mem_cached == -1) {
		return -1;
	}

	// 如果内核没有提供 MemAvailable（旧内核），估算一个值
	if (mem_avail == -1) {
		mem_avail = mem_free + mem_buffers + mem_cached;
	}

	// 边界检查：确保 mem_avail <= mem_total
	if (mem_avail > mem_total) {
		mem_avail = mem_total;
	}

	mem->total = mem_total;
	mem->in_use = mem_total - mem_avail;
	mem->rate = (mem_total > 0) ? ((double) mem->in_use) / mem_total : 0.0;

	return 0;
}

/* ==================== 生产环境入口 ==================== */

int mem_usage(resource_usage *mem) {
	if (!mem) { return -1; }

	FILE *stream = fopen("/proc/meminfo", "r");
	if (!stream) { return -1; }

	int result = parse_meminfo(stream, mem);
	fclose(stream);
	return result;
}
