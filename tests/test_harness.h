#ifndef _TEST_HARNESS_H_
#define _TEST_HARNESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

/* 测试断言宏 */
#define ASSERT_TRUE(cond) do { \
	if (!(cond)) { \
		fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
		tests_failed++; \
		return; \
	} \
} while(0)

#define ASSERT_FALSE(cond) do { \
	if (cond) { \
		fprintf(stderr, "  FAIL: %s:%d: NOT %s\n", __FILE__, __LINE__, #cond); \
		tests_failed++; \
		return; \
	} \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
	if ((expected) != (actual)) { \
		fprintf(stderr, "  FAIL: %s:%d: %s == %s (expected %lld, got %lld)\n", \
		        __FILE__, __LINE__, #expected, #actual, \
		        (long long)(expected), (long long)(actual)); \
		tests_failed++; \
		return; \
	} \
} while(0)

#define ASSERT_DOUBLE_EQ(expected, actual, epsilon) do { \
	double _e = (expected), _a = (actual); \
	if (fabs(_e - _a) > (epsilon)) { \
		fprintf(stderr, "  FAIL: %s:%d: %s == %s (expected %.6f, got %.6f)\n", \
		        __FILE__, __LINE__, #expected, #actual, _e, _a); \
		tests_failed++; \
		return; \
	} \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
	if (strcmp((expected), (actual)) != 0) { \
		fprintf(stderr, "  FAIL: %s:%d: %s == %s (expected \"%s\", got \"%s\")\n", \
		        __FILE__, __LINE__, #expected, #actual, (expected), (actual)); \
		tests_failed++; \
		return; \
	} \
} while(0)

/* 测试运行器 */
static int tests_run = 0;
static int tests_failed = 0;
static int tests_passed = 0;

#define RUN_TEST(test_func) do { \
	int prev_failed = tests_failed; \
	tests_run++; \
	fprintf(stderr, "Running %s...\n", #test_func); \
	test_func(); \
	if (tests_failed == prev_failed) { \
		tests_passed++; \
	} \
} while(0)

/* 打印测试结果 */
#define PRINT_TEST_RESULTS() do { \
	fprintf(stderr, "\n========================================\n"); \
	fprintf(stderr, "Tests run: %d\n", tests_run); \
	fprintf(stderr, "Passed: %d\n", tests_passed); \
	fprintf(stderr, "Failed: %d\n", tests_failed); \
	fprintf(stderr, "========================================\n"); \
	if (tests_failed > 0) { \
		fprintf(stderr, "RESULT: FAILED\n"); \
		exit(1); \
	} else { \
		fprintf(stderr, "RESULT: PASSED\n"); \
		exit(0); \
	} \
} while(0)

/* 测试辅助函数：创建临时文件 */
static char *create_temp_file_with_content(const char *content, size_t len) {
	char *path = strdup("/tmp/test_resource_XXXXXX");
	int fd = mkstemp(path);
	if (fd < 0) {
		free(path);
		return NULL;
	}
	if (content && len > 0) {
		if (write(fd, content, len) < 0) {
			close(fd);
			unlink(path);
			free(path);
			return NULL;
		}
	}
	close(fd);
	return path;
}

/* 测试辅助函数：删除临时文件 */
static void remove_temp_file(char *path) {
	if (path) {
		unlink(path);
		free(path);
	}
}

/* 测试辅助函数：从字符串创建 FILE* */
static FILE *open_string_stream(const char *content) {
	return fmemopen((void *)content, strlen(content), "r");
}

#endif /* _TEST_HARNESS_H_ */
