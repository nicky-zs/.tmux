#ifndef _USAGE_H_
#define _USAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
void cleanup_cpu_tempfile(void);
extern char *cpu_tempfile;

/* 显示函数 - 暴露给测试 */
const char *get_color_for_rate(double rate);
void display_cpu(resource_usage *cpu, const char *name, int narrow_mode);
char new_unit(unsigned long long kb, double *new_size);
void display_mem(resource_usage *mem, int narrow_mode);

/* ==================== 重构：状态与计算分离 ==================== */

/* 显示状态结构 - 描述一个完整的显示场景 */
typedef struct {
	int narrow_mode;            /* 1=窄屏，0=宽屏 */
	double cpu_rate;            /* 总 CPU 利用率 (0.0-1.0) */
	double max_core_rate;       /* 最高单核利用率 (0.0-1.0)，单核时为 0 */
	int core_count;             /* CPU 核心数 */
	double mem_rate;            /* 内存利用率 (0.0-1.0) */
	unsigned long long mem_total_kb; /* 内存总量 (KB) */
} display_state;

/* 纯函数：格式化状态行，输出到缓冲区 */
void format_status_line(const display_state *state, char *buffer, size_t bufsize);

/* 纯函数：格式化 CPU 部分，输出到缓冲区 */
void format_cpu_segment(const char *emoji, double rate, int narrow_mode, char *buffer, size_t bufsize);

/* 纯函数：格式化内存部分，输出到缓冲区 */
void format_mem_segment(double rate, unsigned long long total_kb, int narrow_mode, char *buffer, size_t bufsize);

/* 工具函数：根据利用率返回颜色代码 */
const char *color_code_for_rate(double rate);

/* ==================== 终端渲染：tmux 控制码 -> ANSI 转义序列 ==================== */

/* 将 tmux 控制代码转换为 ANSI 转义序列，输出到缓冲区 */
void render_tmux_codes_to_ansi(const char *tmux_input, char *buffer, size_t bufsize);

/* 在终端打印带颜色的状态行（用于肉眼验证） */
void print_status_line_colored(const display_state *state);

/* ==================== 统一入口：完整状态行（资源栏 + 时间栏） ==================== */
/* 格式化时间栏部分 */
void format_time_segment(int wide_mode, char *buffer, size_t bufsize);

/* 生成完整的状态行（资源栏 + 时间栏），供测试和生产环境统一调用 */
void format_full_status_line(const display_state *state, int wide_mode, char *buffer, size_t bufsize);

/* 工具函数 */
static inline int starts_with(const char *str, const char *prefix) {
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

#endif
