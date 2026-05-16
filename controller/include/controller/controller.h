#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "controller/comms.h"
#include "controller/logging.h"
#include "controller/scheduling.h"
#include "controller/storage.h"
#include <stdint.h>

typedef enum {
  CONTROLLER_RESULT_OK = 0,
  CONTROLLER_RESULT_ERROR_UNKNOWN = -1,
  CONTROLLER_RESULT_ERROR_INIT_FAILED = -2,
  CONTROLLER_RESULT_ERROR_DEINIT_FAILED = -3,
  CONTROLLER_RESULT_ERROR_STEP_FAILED = -4,
} controller_result_t;

typedef struct {
  log_level_t log_level;
} controller_config_t;

extern controller_config_t controller_config;

typedef struct {
  epoch_t epoch;
  comms_t comms;
  storage_t storage;
} controller_t;

/// @defgroup CONTROLLER_methods CONTROLLER methods
/// @{

/// Initialize the controller
/// @brief Initialize the controller
/// @param controller Controller
/// @return CONTROLLER_RESULT_OK if the controller was initialized,
/// CONTROLLER_RESULT_ERROR_UNKNOWN if an error occurred
controller_result_t controller_init(controller_t *controller);

/// Deinitialize the controller
/// @brief Deinitialize the controller
/// @param controller Controller
/// @return CONTROLLER_RESULT_OK if the controller was deinitialized,
/// CONTROLLER_RESULT_ERROR_UNKNOWN if an error occurred
controller_result_t controller_deinit(controller_t *controller);

/// Step the controller
/// @brief Step the controller
/// @param controller Controller
/// @return CONTROLLER_RESULT_OK if the controller was stepped,
/// CONTROLLER_RESULT_ERROR_UNKNOWN if an error occurred
controller_result_t controller_step(controller_t *controller);
/// @}
#endif