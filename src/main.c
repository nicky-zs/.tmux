#include "resource-usage.h"

/* 声明外部函数 */
extern void init_cpu_tempfile(const char *tmux_env);
extern int resource_usage_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	return resource_usage_main(argc, argv);
}
