#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "resource-usage.h"

char *cpu_tempfile = NULL;

void init_cpu_tempfile(const char *tmux_env) {
	if (cpu_tempfile) return;

	// $TMUCK format: /socket/path,server-pid,session-id
	// e.g., /tmp/tmux-1000/default,6490,0
	// We use socket basename + session_id to uniquely identify each session
	const char *comma = strrchr(tmux_env, ',');
	const char *session_id = (comma && *(comma + 1)) ? (comma + 1) : NULL;

	char unique_id[256];
	if (session_id) {
		// Get socket path basename (between last '/' and first ',')
		const char *slash = strrchr(tmux_env, '/');
		const char *basename = slash ? (slash + 1) : tmux_env;
		comma = strchr(basename, ',');
		size_t blen = comma ? (size_t)(comma - basename) : strlen(basename);

		if (blen >= sizeof(unique_id)) blen = sizeof(unique_id) - 1;
		memcpy(unique_id, basename, blen);
		snprintf(unique_id + blen, sizeof(unique_id) - blen, ",%s", session_id);
	} else {
		// Fallback: use full string (truncated if needed)
		strncpy(unique_id, tmux_env, sizeof(unique_id) - 1);
		unique_id[sizeof(unique_id) - 1] = '\0';
	}

	size_t len = strlen("/tmp/resource-usage.cpu..data") + strlen(unique_id) + 1;
	cpu_tempfile = malloc(len);
	if (!cpu_tempfile) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	snprintf(cpu_tempfile, len, "/tmp/resource-usage.cpu.%s.data", unique_id);
}

/* ==================== 记录列表操作 ==================== */

int record_list_init(record_list *list, size_t init_cap) {
	if (!list || init_cap <= 0) { return -1; }
	if (!(list->elem = malloc(init_cap * sizeof(cpu_record)))) { return -1; }
	list->len = 0;
	list->cap = init_cap;
	return 0;
}

int record_list_load(record_list *list, const char *path) {
	list->cap = 0;
	list->len = 0;
	list->elem = NULL;

	FILE *stream = fopen(path, "rb");  // 二进制模式
	if (!stream) { return -1; }

	if (fread(&list->len, sizeof(list->len), 1, stream) != 1) {
		fclose(stream);
		return -1;
	}

	// 合理性检查：CPU 核心数不可能超过 1024
	if (list->len > 1024) {
		fclose(stream);
		return -1;
	}

	if (!(list->elem = malloc(list->len * sizeof(*list->elem)))) {
		fclose(stream);
		return -1;
	}
	list->cap = list->len;

	if (fread(list->elem, sizeof(*list->elem), list->len, stream) != list->len) {
		free(list->elem);
		list->elem = NULL;
		list->cap = 0;
		list->len = 0;
		fclose(stream);
		return -1;
	}

	fclose(stream);
	return 0;
}

int record_list_dump(record_list *list, const char *path) {
	FILE *stream = fopen(path, "wb");  // 二进制模式
	if (!stream) { return -1; }

	if (fwrite(&list->len, sizeof(list->len), 1, stream) != 1) {
		fclose(stream);
		return -1;
	}
	if (fwrite(list->elem, sizeof(*list->elem), list->len, stream) != list->len) {
		fclose(stream);
		return -1;
	}

	fclose(stream);
	return 0;
}

void record_list_destroy(record_list *list) {
	free(list->elem);
	list->elem = NULL;
	list->len = 0;
	list->cap = 0;
}

int record_list_add(record_list *list, cpu_record *record) {
	if (list->len == list->cap) {
		size_t cap = list->cap ? list->cap * 2 : 128;
		cpu_record *elem = realloc(list->elem, cap * sizeof(cpu_record));
		if (!elem) { return -1; }
		list->elem = elem;
		list->cap = cap;
	}
	list->elem[list->len].busy = record->busy;
	list->elem[list->len].total = record->total;
	list->len++;
	return 0;
}

/* ==================== CPU 统计解析 - 可测试 ==================== */

int read_proc_stat_from_stream(FILE *stream, record_list *list) {
	if (!list || !stream) { return -1; }

	size_t line_size = 0;
	char *line = NULL;
	char *token, *saveptr, *delim = " :\n";
	int found_cpu = 0;

	while (getline(&line, &line_size, stream) != -1) {
		token = strtok_r(line, delim, &saveptr);
		if (!token) continue;

		// 只处理 "cpu" 行（总体统计）和 "cpuN" 行（单核统计）
		if (strncmp(token, "cpu", 3) != 0) continue;

		// 确保是 "cpu" 或 "cpuN" 格式
		if (token[3] != '\0' && (token[3] < '0' || token[3] > '9')) continue;

		found_cpu = 1;
		cpu_record record = { 0, 0 };
		int i = 0;
		while ((token = strtok_r(NULL, delim, &saveptr))) {
			i++;
			// user(1) + nice(2) + system(3) + idle(4) + iowait(5) + irq(6) + softirq(7) + ...
			// busy = user + nice + system + irq + softirq + ...
			// total = 所有字段之和
			if (i != 4 && i != 5) {  // 跳过 idle 和 iowait
				record.busy += strtoull(token, NULL, 10);
			}
			record.total += strtoull(token, NULL, 10);
		}
		record_list_add(list, &record);
	}

	free(line);
	return found_cpu ? 0 : -1;
}

/* 生产环境：直接从 /proc/stat 读取 */
int read_proc_stat_file(record_list *list) {
	if (!list) { return -1; }

	FILE *stream = fopen("/proc/stat", "r");
	if (!stream) { return -1; }

	int result = read_proc_stat_from_stream(stream, list);
	fclose(stream);
	return result;
}

/* ==================== CPU 使用率计算 - 纯函数，可测试 ==================== */

int compute_cpu_usage(record_list *prev, record_list *current,
                      resource_usage *cpu, resource_usage *max_core, int *cores) {
	if (!prev || !current || !cpu || !max_core || !cores) { return -1; }
	if (current->len == 0) { return -1; }
	if (prev->len != current->len) { return -2; }  // 记录数不匹配

	*cores = current->len - 1;

	// 计算总体 CPU 使用率
	cpu->in_use = current->elem[0].busy - prev->elem[0].busy;
	cpu->total = current->elem[0].total - prev->elem[0].total;
	cpu->rate = (cpu->total > 0) ? ((double) cpu->in_use) / cpu->total : 0.0;

	// 找出使用率最高的单核
	size_t max_index = 1;
	double max_rate = 0.0;
	for (size_t i = 1; i < current->len; i++) {
		unsigned long long in_use = current->elem[i].busy - prev->elem[i].busy;
		unsigned long long total = current->elem[i].total - prev->elem[i].total;
		double rate = (total > 0) ? ((double) in_use) / total : 0.0;
		if (rate > max_rate) {
			max_rate = rate;
			max_index = i;
		}
	}

	max_core->in_use = current->elem[max_index].busy - prev->elem[max_index].busy;
	max_core->total = current->elem[max_index].total - prev->elem[max_index].total;
	max_core->rate = max_rate;

	return 0;
}

/* ==================== 生产环境入口 ==================== */

static inline void zero(resource_usage *res) {
	res->in_use = 0;
	res->total = 0;
	res->rate = 0.0;
}

int cpu_usage(resource_usage *cpu, resource_usage *max_core, int *cores) {
	if (!cpu || !max_core || !cores) { return -1; }

	record_list prev = { 0 };
	record_list current = { 0 };
	int no_prev = 0;

	if (record_list_load(&prev, cpu_tempfile) == -1) {
		no_prev = 1;
	}

	if (record_list_init(&current, 128) == -1) {
		record_list_destroy(&prev);
		return -1;
	}

	if (read_proc_stat_file(&current) == -1) {
		record_list_destroy(&prev);
		record_list_destroy(&current);
		return -1;
	}

	// 如果记录数不匹配（首次运行或 CPU 热插拔），保存当前数据但不计算使用率
	if (prev.len != current.len) {
		no_prev = 1;
	}

	// 保存当前数据供下次使用
	record_list_dump(&current, cpu_tempfile);

	if (no_prev) {
		zero(cpu);
		zero(max_core);
		*cores = current.len - 1;
		record_list_destroy(&prev);
		record_list_destroy(&current);
		return 0;
	}

	int result = compute_cpu_usage(&prev, &current, cpu, max_core, cores);

	record_list_destroy(&prev);
	record_list_destroy(&current);
	return (result == 0 || result == -2) ? 0 : -1;
}
