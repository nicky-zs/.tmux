#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "resource-usage.h"

#define CPU_WARNING_THRESHOLD 0.60
#define CPU_CRITICAL_THRESHOLD 0.85

static inline void exit_on_error(const char *msg) {
	errno ? perror(msg) : fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

/* ==================== 显示格式化 - 纯函数，可测试 ==================== */

const char *color_code_for_rate(double rate) {
	if (rate >= CPU_CRITICAL_THRESHOLD) {
		return "#[bg=colour1,fg=colour255]";
	}
	if (rate >= CPU_WARNING_THRESHOLD) {
		return "#[bg=colour3,fg=colour255]";
	}
	return "#[bg=colour10,fg=colour255]";  /* 正常时绿色背景 */
}

const char *get_color_for_rate(double rate) {
	return color_code_for_rate(rate);
}

static const char *color(double rate) {
	if (rate == 0) {
		return "#[bg=colour10,fg=colour255]";  /* 重置为绿色背景 */
	}
	return color_code_for_rate(rate);
}

/* ==================== 新纯函数接口：状态 -> 字符串 ==================== */

void format_cpu_segment(const char *emoji, double rate, int narrow_mode, char *buffer, size_t bufsize) {
	if (!buffer || bufsize == 0) return;

	if (narrow_mode) {
		/* 窄屏：带 emoji，紧凑格式 "📊XX.X%|" */
		snprintf(buffer, bufsize, "%s%s%3.1f%%%s|",
		         color(rate), emoji, 100.0 * rate, color(0));
	} else {
		/* 宽屏：带 emoji，格式 " 📊XX.X%" */
		snprintf(buffer, bufsize, "%s %s%s%3.1f%%%s",
		         color(0), color(rate), emoji, 100.0 * rate, color(0));
	}
}

static const char units[] = "KMGTPEZY";

char new_unit(unsigned long long kb, double *new_size) {
	double size = kb;
	const char *p = units;
	while (size >= 1000) {
		size /= 1024;
		if (!*++p) {
			fprintf(stderr, "memory too large\n");
			exit(EXIT_FAILURE);
		}
	}
	*new_size = size;
	return *p;
}

void format_mem_segment(double rate, unsigned long long total_kb, int narrow_mode, char *buffer, size_t bufsize) {
	if (!buffer || bufsize == 0) return;
	if (total_kb == 0) {
		buffer[0] = '\0';
		return;
	}

	double in_use, total;
	double new_size_double;

	(void)new_unit(total_kb, &new_size_double);
	total = new_size_double;
	in_use = rate * total;

	if (narrow_mode) {
		/* 窄屏：带㎇符号，紧凑格式 "㎇XX.X/XX.X" */
		snprintf(buffer, bufsize, "%s㎇%.1f/%.1f%s",
		         color(rate), in_use, total, color(0));
	} else {
		/* 宽屏：格式 " ㎇XX.X/XX.X " - ㎇跟着数值一起高亮 */
		snprintf(buffer, bufsize, "%s %s㎇%.1f/%.1f%s ",
		         color(0), color(rate), in_use, total, color(0));
	}
}

void format_status_line(const display_state *state, char *buffer, size_t bufsize) {
	if (!state || !buffer || bufsize == 0) return;

	char cpu_buf[128] = {0};
	char max_core_buf[128] = {0};
	char mem_buf[128] = {0};

	/* 格式化 CPU 部分 */
	format_cpu_segment("📊", state->cpu_rate, state->narrow_mode, cpu_buf, sizeof(cpu_buf));

	/* 格式化最高单核部分（多核时始终显示） */
	if (state->core_count > 1) {
		format_cpu_segment("📈", state->max_core_rate, state->narrow_mode, max_core_buf, sizeof(max_core_buf));
	}

	/* 格式化内存部分 */
	format_mem_segment(state->mem_rate, state->mem_total_kb, state->narrow_mode, mem_buf, sizeof(mem_buf));

	/* 窄屏模式：无前导空格，资源栏紧凑 */
	snprintf(buffer, bufsize, "%s%s%s", cpu_buf, max_core_buf, mem_buf);
}

/* 格式化时间栏部分 */
void format_time_segment(int wide_mode, char *buffer, size_t bufsize) {
	if (!buffer || bufsize == 0) return;

	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);

	/* 星期几的缩写 */
	static const char *wdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	if (wide_mode) {
		/* 宽屏时间格式：#[bg=colour6] Fri 2026/04/06 09:00:00  */
		snprintf(buffer, bufsize, "#[bg=colour6] %s %04d/%02d/%02d %02d:%02d:%02d ",
		         wdays[tm_info->tm_wday],
		         tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
		         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
	} else {
		/* 窄屏时间格式：#[bg=colour6] 04/06 09:00:00  */
		snprintf(buffer, bufsize, "#[bg=colour6] %02d/%02d %02d:%02d:%02d ",
		         tm_info->tm_mon + 1, tm_info->tm_mday,
		         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
	}
}

/* 生成完整的状态行（资源栏 + 时间栏），供测试和生产环境统一调用 */
void format_full_status_line(const display_state *state, int wide_mode, char *buffer, size_t bufsize) {
	if (!state || !buffer || bufsize == 0) return;

	char resource_buf[512];
	char time_buf[128];

	/* 生成资源栏 */
	format_status_line(state, resource_buf, sizeof(resource_buf));

	/* 生成时间栏 */
	format_time_segment(wide_mode, time_buf, sizeof(time_buf));

	/* 拼接 */
	snprintf(buffer, bufsize, "%s%s", resource_buf, time_buf);
}

/* ==================== 兼容旧接口（测试使用） ==================== */

void display_cpu(resource_usage *cpu, const char *name, int narrow_mode) {
	if (!cpu) { return; }
	char buffer[128];
	format_cpu_segment(name, cpu->rate, narrow_mode, buffer, sizeof(buffer));
	printf("%s", buffer);
}

void display_mem(resource_usage *mem, int narrow_mode) {
	if (!mem || mem->total == 0) { return; }
	char buffer[128];
	format_mem_segment(mem->rate, mem->total, narrow_mode, buffer, sizeof(buffer));
	printf("%s", buffer);
}

/* ==================== 终端渲染：tmux 控制码 -> ANSI 转义序列 ==================== */

/* tmux 颜色码 -> ANSI 256 色码 */
static int tmux_colour_to_ansi(int colour) {
	/* 标准 colour0-15 */
	static const int standard_colours[] = {
		0,   /* colour0 -> black */
		1,   /* colour1 -> red */
		2,   /* colour2 -> green */
		3,   /* colour3 -> yellow */
		4,   /* colour4 -> blue */
		5,   /* colour5 -> magenta */
		6,   /* colour6 -> cyan */
		7,   /* colour7 -> white */
		8,   /* colour8 -> bright black (dark grey) */
		9,   /* colour9 -> bright red */
		10,  /* colour10 -> bright green */
		11,  /* colour11 -> bright yellow */
		12,  /* colour12 -> bright blue */
		13,  /* colour13 -> bright magenta */
		14,  /* colour14 -> bright cyan */
		15,  /* colour15 -> bright white */
	};

	if (colour >= 0 && colour <= 15) {
		return standard_colours[colour];
	}
	/* 16-255 直接使用 256 色模式 */
	return colour;
}

/* 解析 tmux 颜色代码并转换为 ANSI 转义序列 */
void render_tmux_codes_to_ansi(const char *tmux_input, char *buffer, size_t bufsize) {
	if (!tmux_input || !buffer || bufsize == 0) return;

	const char *p = tmux_input;
	char *out = buffer;
	char *out_end = buffer + bufsize - 1;

	while (*p && out < out_end) {
		/* 检查 tmux 控制码：#[bg=colourX,fg=colourY] */
		if (strncmp(p, "#[bg=", 5) == 0) {
			p += 5;  /* 跳过 "#[bg=" */

			/* 解析背景色 */
			int bg_colour = -1;
			if (strncmp(p, "colour", 6) == 0) {
				p += 6;
				bg_colour = atoi(p);
				while (*p >= '0' && *p <= '9') p++;
			}

			/* 查找逗号分隔符 */
			while (*p && *p != ',' && *p != ']') p++;
			if (*p == ',') p++;  /* 跳过逗号 */

			/* 解析前景色 */
			int fg_colour = -1;
			if (strncmp(p, "fg=", 3) == 0) {
				p += 3;
				if (strncmp(p, "colour", 6) == 0) {
					p += 6;
					fg_colour = atoi(p);
					while (*p >= '0' && *p <= '9') p++;
				}
			}

			/* 跳过结束括号 */
			while (*p && *p != ']') p++;
			if (*p == ']') p++;

			/* 输出 ANSI 转义序列：\033[48;5;BgColor;38;5;FgColor m */
			if (bg_colour >= 0 || fg_colour >= 0) {
				int n = snprintf(out, out_end - out + 1, "\033[");
				if (out + n > out_end) break;
				out += n;

				if (bg_colour >= 0) {
					n = snprintf(out, out_end - out + 1, "48;5;%d", tmux_colour_to_ansi(bg_colour));
					if (out + n > out_end) break;
					out += n;
				}

				if (fg_colour >= 0) {
					if (bg_colour >= 0) {
						n = snprintf(out, out_end - out + 1, ";");
						if (out + n > out_end) break;
						out += n;
					}
					n = snprintf(out, out_end - out + 1, "38;5;%d", tmux_colour_to_ansi(fg_colour));
					if (out + n > out_end) break;
					out += n;
				}

				n = snprintf(out, out_end - out + 1, "m");
				if (out + n > out_end) break;
				out += n;
			}
		} else if (*p == '\033') {
			/* 已经是 ANSI 转义序列，直接复制 */
			*out++ = *p++;
			while (*p && *p != 'm' && out < out_end) {
				*out++ = *p++;
			}
			if (*p == 'm' && out < out_end) {
				*out++ = *p++;
			}
		} else {
			/* 普通字符 */
			*out++ = *p++;
		}
	}

	*out = '\0';
}

/* 在终端打印带颜色的状态行（用于肉眼验证） */
void print_status_line_colored(const display_state *state) {
	char buffer[512];
	char ansi_buffer[1024];

	/* 生成 tmux 格式的状态行 */
	format_status_line(state, buffer, sizeof(buffer));

	/* 转换为 ANSI 转义序列 */
	render_tmux_codes_to_ansi(buffer, ansi_buffer, sizeof(ansi_buffer));

	/* 打印带颜色的输出 */
	printf("%s\033[0m", ansi_buffer);  /* \033[0m 重置颜色 */
}

/* ==================== 生产环境入口 ==================== */

int resource_usage_main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <tmux_socket_path> [narrow]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Initialize temp file path based on tmux socket
	init_cpu_tempfile(argv[1]);

	int narrow_mode = 0;
	if (argc > 2 && strcmp(argv[2], "narrow") == 0) {
		narrow_mode = 1;
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

	/* 使用统一函数生成完整的状态行（资源栏 + 时间栏） */
	display_state state = {
		.narrow_mode = narrow_mode,
		.cpu_rate = cpu.rate,
		.max_core_rate = max_core.rate,
		.core_count = cores,
		.mem_rate = mem.rate,
		.mem_total_kb = mem.total,
	};

	char buffer[1024];
	format_full_status_line(&state, !narrow_mode, buffer, sizeof(buffer));
	printf("%s\n", buffer);
	fflush(stdout);

	return 0;
}
