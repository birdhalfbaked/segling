#include "controller/controller.h"
#include "controller/logging.h"
#include <string.h>

static void parse_args(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      controller_config.log_level = LOG_LEVEL_DEBUG;
    }
  }
}

int main(int argc, char *argv[]) {
  parse_args(argc, argv);

  controller_t controller;
  controller_init(&controller);
  while (1) {
    controller_step(&controller);
  }
  controller_deinit(&controller);
  return 0;
}