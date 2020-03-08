#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <argp.h>

#include "resource-usage.h"

static int narrow = 0;

static inline void exit_on_error(const char *msg) {
	errno ? perror(msg) : fprintf(stderr, "%s\n", msg);
	exit(-1);
}

static inline const char *color(double rate) {
	if (rate > 0.8) {
		return "#[bg=colour1,fg=colour255]";
	}
	if (rate > 0.3) {
		return "#[bg=colour3,fg=colour255]";
	}
	return "#[bg=colour10,fg=colour255]";
}

static inline void display_cpu(resource_usage *cpu, const char *name) {
	if (!cpu) { return; }
	if (narrow) {
		printf("%s%3.1f%%%s|", color(cpu->rate), 100.0 * cpu->rate, color(0));
	} else {
		printf("%s %s:%3.1f%% %s", color(cpu->rate), name, 100.0 * cpu->rate, color(0));
	}
}

static const char units[] = "KMGTPEZY";

static inline char new_unit(unsigned long long kb, double *new_size) {
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

static inline void display_mem(resource_usage *mem) {
	double in_use, total;
	double rate = ((double) mem->in_use) / mem->total;
	char unit_in_use = new_unit(mem->in_use, &in_use);
	char unit_total = new_unit(mem->total, &total);

	if (narrow) {
		printf("%s%3.1f%c/%3.1f%c%s", color(rate), in_use, unit_in_use, total, unit_total, color(0));
	} else {
		printf("%s %3.1f%cB/%3.1f%cB %s", color(rate), in_use, unit_in_use, total, unit_total, color(0));
	}
}

int main(int argc, char *argv[]) {
	if (argc > 1 && strcmp(argv[1], "narrow") == 0) {
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

	display_cpu(&cpu, "CPU");
	if (cores > 1) {
		display_cpu(&max_core, "Core");
	}
	display_mem(&mem);

	printf("\n");
	fflush(stdout);
	return 0;
}
