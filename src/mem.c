#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource-usage.h"

int mem_usage(resource_usage *mem) {
	if (!mem) { return -1; }

	FILE *stream = fopen("/proc/meminfo", "r");
	if (!stream) { return -1; }

	long long mem_total = -1, mem_free = -1, mem_avail = -1, mem_buffers = -1, mem_cached = -1;

	size_t n = 0;
	char *line = NULL;
	char *token, *saveptr, *delim = " :";
	while (getline(&line, &n, stream) != -1 && (token = strtok_r(line, delim, &saveptr))) {
		if (equals(token, "MemTotal")) {
			mem_total = strtoull(strtok_r(NULL, delim, &saveptr), NULL, 10);

		} else if (equals(token, "MemFree")) {
			mem_free = strtoull(strtok_r(NULL, delim, &saveptr), NULL, 10);

		} else if (equals(token, "MemAvailable")) {
			mem_avail = strtoull(strtok_r(NULL, delim, &saveptr), NULL, 10);

		} else if (equals(token, "Buffers")) {
			mem_buffers = strtoull(strtok_r(NULL, delim, &saveptr), NULL, 10);

		} else if (equals(token, "Cached")) {
			mem_cached = strtoull(strtok_r(NULL, delim, &saveptr), NULL, 10);
		}
		// else break?
	}
	free(line);
	fclose(stream);

	if (mem_total == -1 || mem_free == -1 || mem_buffers == -1 || mem_cached == -1) {
		return -1;
	}

	if (mem_avail == -1) {
		mem_avail = mem_free + mem_buffers + mem_cached;
	}

	mem->total = mem_total;
	mem->in_use = mem_total - mem_avail;
	mem->rate = 100.0 * mem->in_use / mem_total;
	return 0;
}
