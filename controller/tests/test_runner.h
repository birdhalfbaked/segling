#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stddef.h>
#include <stdint.h>

typedef int (*test_fn)(void);

typedef struct {
  const char *suite;
  const char *name;
  test_fn fn;
} test_entry_t;

#define TEST_REGISTRY_CAP 256

typedef struct {
  test_entry_t entries[TEST_REGISTRY_CAP];
  size_t count;
} test_registry_t;

void test_registry_add(test_registry_t *reg, const char *suite,
                     const char *name, test_fn fn);
/// Runs every registered test; returns number of failures.
int test_registry_run_all(const test_registry_t *reg);

#define TEST(name) static int test_##name(void)

#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      test_fail_assertion(#cond, __FILE__, __LINE__);                          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_EQ_I32(a, b) ASSERT((int32_t)(a) == (int32_t)(b))

void test_fail_assertion(const char *expr, const char *file, int line);

#endif
