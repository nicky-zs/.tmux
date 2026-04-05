#ifndef _USAGE_H_
#define _USAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct {
	unsigned long long busy;
	unsigned long long total;
} cpu_record;

typedef struct {
	unsigned long long in_use;
	unsigned long long total;
	double rate;
} resource_usage;

/* CPU 记录列表 - 用于测试 */
typedef struct {
	size_t len;
	size_t cap;
	cpu_record *elem;
} record_list;

/* 记录列表操作 - 暴露给测试 */
int record_list_init(record_list *list, size_t init_cap);
int record_list_load(record_list *list, const char *path);
int record_list_dump(record_list *list, const char *path);
void record_list_destroy(record_list *list);
int record_list_add(record_list *list, cpu_record *record);

/* 依赖注入接口 - 用于测试时 mock 输入 */
typedef FILE *(*file_opener)(const char *path, const char *mode);
typedef int (*file_reader)(FILE *stream, record_list *list);
typedef int (*file_writer)(record_list *list, const char *path);

/* 核心计算逻辑 - 可测试 */
int read_proc_stat_from_stream(FILE *stream, record_list *list);
int compute_cpu_usage(record_list *prev, record_list *current,
                      resource_usage *cpu, resource_usage *max_core, int *cores);
int parse_meminfo(FILE *stream, resource_usage *mem);

/* 文件操作 - 生产环境使用 */
int read_proc_stat_file(record_list *list);
int load_records(record_list *list, const char *path);
int save_records(record_list *list, const char *path);

/* 生产环境入口 */
int cpu_usage(resource_usage *cpu, resource_usage *max_core, int *cores);
int mem_usage(resource_usage *mem);

/* CPU 临时文件初始化（来自 cpu.c） */
void init_cpu_tempfile(const char *tmux_env);
extern char *cpu_tempfile;

/* 显示函数 - 暴露给测试 */
const char *get_color_for_rate(double rate);
void display_cpu(resource_usage *cpu, const char *name, int narrow_mode);
char new_unit(unsigned long long kb, double *new_size);
void display_mem(resource_usage *mem, int narrow_mode);

/* 工具函数 */
#define starts_with(str, prefix) (strncmp((str), (prefix), sizeof(prefix) - 1) == 0)

#endif
