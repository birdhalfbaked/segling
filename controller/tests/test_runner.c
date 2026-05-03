#include "test_runner.h"
#include <stdio.h>

void test_fail_assertion(const char *expr, const char *file, int line) {
  fprintf(stderr, "    assertion failed: %s (%s:%d)\n", expr, file, line);
}

void test_registry_add(test_registry_t *reg, const char *suite,
                       const char *name, test_fn fn) {
  if (reg == NULL || suite == NULL || name == NULL || fn == NULL) {
    return;
  }
  if (reg->count >= TEST_REGISTRY_CAP) {
    fprintf(stderr, "test_registry_add: capacity exceeded (%s::%s)\n", suite,
            name);
    return;
  }
  reg->entries[reg->count++] =
      (test_entry_t){.suite = suite, .name = name, .fn = fn};
}

int test_registry_run_all(const test_registry_t *reg) {
  if (reg == NULL) {
    return 1;
  }
  int failures = 0;
  for (size_t i = 0; i < reg->count; i++) {
    const test_entry_t *e = &reg->entries[i];
    printf("  [%s] %s ... ", e->suite, e->name);
    fflush(stdout);
    if (e->fn() != 0) {
      failures++;
      printf("FAIL\n");
    } else {
      printf("ok\n");
    }
  }
  return failures;
}
