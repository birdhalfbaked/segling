/**
 * Master test entrypoint: register suites from other compilation units, then run.
 */

#include "test_runner.h"
#include <stdio.h>

extern void ahrs_calibration_register_tests(test_registry_t *reg);

static void register_all_tests(test_registry_t *reg) {
  ahrs_calibration_register_tests(reg);
}

int main(void) {
  test_registry_t reg = {0};
  register_all_tests(&reg);

  printf("controller tests (%zu registered)\n\n", reg.count);

  int failures = test_registry_run_all(&reg);

  if (failures != 0) {
    printf("\n%d test(s) failed\n", failures);
    return 1;
  }
  printf("\nall tests passed\n");
  return 0;
}
