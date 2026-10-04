#ifndef TEST_ALLOC_HOOKS_H
#define TEST_ALLOC_HOOKS_H
#include <stddef.h>
void *test_malloc(size_t size);
void test_free(void *pointer);
#endif
