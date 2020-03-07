#ifndef _USAGE_H_
#define _USAGE_H_

typedef struct {
	unsigned long long in_use;
	unsigned long long total;
	double rate;
} resource_usage;

int cpu_usage(resource_usage *cpu, resource_usage *max_core, int *cores);
int mem_usage(resource_usage *mem);

#define equals(a, b) (strncmp((a), (b), sizeof(b) - 1) == 0)

#endif
