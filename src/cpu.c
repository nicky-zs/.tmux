#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "resource-usage.h"

char *cpu_tempfile = NULL;

void init_cpu_tempfile(const char *tmux_env) {
	if (cpu_tempfile) return;

	// $TMUX format: /socket/path,server-pid,session-id
	// e.g., /tmp/tmux-1000/default,6490,0
	// We use the full string to uniquely identify each tmux pane/session
	const char *p = tmux_env;
	// Find the last comma to get session-id
	const char *session_id = NULL;
	while (*p) {
		if (*p == ',') session_id = p + 1;
		p++;
	}

	// If we have a session ID, use socket_basename + session_id
	// Otherwise fall back to full tmux env string hash
	char unique_id[256];
	if (session_id && *session_id) {
		// Get socket basename
		const char *basename = tmux_env;
		p = tmux_env;
		while (*p) {
			if (*p == '/') basename = p + 1;
			// Stop at first comma
			if (*p == ',') break;
			p++;
		}
		// Copy basename
		size_t blen = p - basename;
		if (blen >= sizeof(unique_id)) blen = sizeof(unique_id) - 1;
		strncpy(unique_id, basename, blen);
		unique_id[blen] = '\0';
		// Append session_id
		snprintf(unique_id + blen, sizeof(unique_id) - blen, ",%s", session_id);
	} else {
		// Fallback: use full string (truncated if needed)
		strncpy(unique_id, tmux_env, sizeof(unique_id) - 1);
		unique_id[sizeof(unique_id) - 1] = '\0';
	}

	// Allocate enough space for /tmp/resource-usage.cpu.<unique_id>.data
	size_t len = strlen("/tmp/resource-usage.cpu..data") + strlen(unique_id) + 1;  // +1 for null terminator
	cpu_tempfile = malloc(len);
	if (!cpu_tempfile) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	snprintf(cpu_tempfile, len, "/tmp/resource-usage.cpu.%s.data", unique_id);
}

typedef struct {
	unsigned long long busy;
	unsigned long long total;
} cpu_record;

typedef struct {
	size_t len;
	size_t cap;
	cpu_record *elem;
} record_list;

static int record_list_init(record_list *list, size_t init_cap) {
	if (!list || init_cap <= 0) { return -1; }
	if (!(list->elem = malloc(init_cap * sizeof(cpu_record)))) { return -1; }
	list->len = 0;
	list->cap = init_cap;
	return 0;
}

static int record_list_load(record_list *list, const char *path) {
	list->cap = 0;
	list->len = 0;
	list->elem = NULL;

	FILE *stream = fopen(path, "r");
	if (!stream) { return -1; }

	if (fread(&list->len, sizeof(list->len), 1, stream) != 1) { fclose(stream); return -1; }
	if (!(list->elem = malloc(list->len * sizeof(*list->elem)))) { fclose(stream); return -1; }
	list->cap = list->len;

	if (fread(list->elem, sizeof(*list->elem), list->len, stream) != list->len) { fclose(stream); return -1; }

	fclose(stream);
	return 0;
}

static int record_list_dump(record_list *list, const char *path) {
	FILE *stream = fopen(path, "w");
	if (!stream) { return -1; }

	if (fwrite(&list->len, sizeof(list->len), 1, stream) != 1) { fclose(stream); return -1; }
	if (fwrite(list->elem, sizeof(*list->elem), list->len, stream) != list->len) { fclose(stream); return -1; }

	fclose(stream);
	return 0;
}

static inline void record_list_destroy(record_list *list) {
	free(list->elem);
	list->elem = NULL;
}

static int record_list_add(record_list *list, cpu_record *record) {
	if (list->len == list->cap) {
		size_t cap = list->cap * 2;
		cpu_record *elem = malloc(cap * sizeof(cpu_record));
		if (!elem) { return -1; }
		memcpy(elem, list->elem, list->len * sizeof(cpu_record));
		free(list->elem);
		list->elem = elem;
	}
	list->elem[list->len].busy = record->busy;
	list->elem[list->len].total = record->total;
	list->len++;
	return 0;
}

static int read_proc_stat(record_list *list) {
	if (!list) { return -1; }

	FILE *stream = fopen("/proc/stat", "r");
	if (!stream) { return -1; }

	size_t ignored = 0;
	char *line = NULL;
	char *token, *saveptr, *delim = " :\n";
	while (getline(&line, &ignored, stream) != -1 && (token = strtok_r(line, delim, &saveptr))) {
		if (!equals(token, "cpu")) { continue; }

		cpu_record record = { 0, 0 };
		int i = 0;
		while ((token = strtok_r(NULL, delim, &saveptr))) {
			i++;
			if (i != 4 && i != 5) {
				record.busy += strtoul(token, NULL, 10);
			}
			record.total += strtoul(token, NULL, 10);
		}
		record_list_add(list, &record);
	}

	free(line);
	fclose(stream);
	return 0;
}

static inline void zero(resource_usage *cpu) {
	cpu->in_use = 0;
	cpu->total = 0;
	cpu->rate = 0.0;
}

int cpu_usage(resource_usage *cpu, resource_usage *max_core, int *cores) {
	if (!cpu || !max_core || !cores) { return -1; }

	record_list prev, current;
	int no_prev = 0;

	if (record_list_load(&prev, cpu_tempfile) == -1) {
		no_prev = 1;
	}
	if (record_list_init(&current, 128) == -1) { record_list_destroy(&prev); return -1; }
	if (read_proc_stat(&current) == -1) { record_list_destroy(&prev); record_list_destroy(&current); return -1; }

	if (current.len != prev.len) {
		record_list_destroy(&prev);
		no_prev = 1;
	}
	if (current.len == 0) { record_list_destroy(&prev); record_list_destroy(&current); return -1; }

	record_list_dump(&current, cpu_tempfile);

	*cores = current.len - 1;

	if (no_prev) {
		zero(cpu);
		zero(max_core);
		record_list_destroy(&prev);
		record_list_destroy(&current);
		return 0;
	}

	cpu->in_use = current.elem[0].busy - prev.elem[0].busy;
	cpu->total = current.elem[0].total - prev.elem[0].total;
	cpu->rate = ((double) cpu->in_use) / cpu->total;

	int max_index = 1;
	double max_rate = 0.0;
	for (int i = 1; i < current.len; i++) {
		unsigned long long in_use = current.elem[i].busy - prev.elem[i].busy;
		unsigned long long total = current.elem[i].total - prev.elem[i].total;
		double rate = ((double) in_use) / total;
		if (rate > max_rate) {
			max_rate = rate;
			max_index = i;
		}
	}

	max_core->in_use = current.elem[max_index].busy - prev.elem[max_index].busy;
	max_core->total = current.elem[max_index].total - prev.elem[max_index].total;
	max_core->rate = max_rate;

	record_list_destroy(&prev);
	record_list_destroy(&current);
	return 0;
}
