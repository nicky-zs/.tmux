#include "resource-usage.h"

/* 声明外部函数 */
extern void init_cpu_tempfile(const char *tmux_env);
extern int resource_usage_main(int argc, char *argv[]);
extern void cleanup_cpu_tempfile(void);

int main(int argc, char *argv[]) {
	int result = resource_usage_main(argc, argv);
	cleanup_cpu_tempfile();
	return result;
}
