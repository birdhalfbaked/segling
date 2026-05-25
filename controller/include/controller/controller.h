/// @defgroup CONTROLLER Controller module
/// @ingroup CORE
/// @brief This module is the main controller module that manages the overall
/// operation of the controller. It is responsible for initializing and
/// deinitializing the controller, as well as stepping the controller.
/// @{
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "controller/comms.h"
#include "controller/logging.h"
#include "controller/scheduling.h"
#include "controller/storage.h"
#include <stdint.h>
#include <time.h>

// Types

/// @brief CONTROLLER result codes
typedef enum {
  /// @brief Success
  CONTROLLER_RESULT_OK = 0,
  /// @brief an unknown or unexpected error has occurred
  CONTROLLER_RESULT_ERROR_UNKNOWN = -1,
  /// @brief the controller could not initialize properly (unrecoverable)
  CONTROLLER_RESULT_ERROR_INIT_FAILED = -2,
  /// @brief the controller could not deinitialize properly (unrecoverable)
  CONTROLLER_RESULT_ERROR_DEINIT_FAILED = -3,
  /// @brief the controller was unable to perform a full epoch round
  /// (recoverable)
  CONTROLLER_RESULT_ERROR_STEP_FAILED = -4,
} controller_result_t;

/// @brief controller configuration structure for parameters that determine
/// runtime behavior of the controller
typedef struct {
  /// @brief Determines the logging level if logging is enabled (Should not be
  /// enabled on deployment to prevent use of stdio)
  log_level_t log_level;
} controller_config_t;

/// @brief controller state and subcomponent structure that enables runtime
/// functionality
typedef struct {
  /// @brief The current epoch maintained by the controller
  epoch_t epoch;
  /// @brief The current slot being processed within one epoch
  slot_id_t current_slot;
  /// @brief The next slot start time in absolute time
  struct timespec next_slot_start_time;
  comms_t comms;
  storage_t storage;
} controller_t;
extern controller_config_t controller_config;

// Functions

/// @brief handler for step slot execution delegation. Each integration
/// component must implement their handlers with the expected signature to
/// allow for epoch validation
/// @memberof controller_t
/// @param controller pointer to the controller_t structure
/// @param epoch the current epoch
/// @pre controller must be a valid pointer to a controller_t structure
/// @pre epoch must be a valid epoch
/// @internal we may not need epoch as a param since we now put this in the
/// controller structure and the deref is cheap
typedef void (*controller_slot_handler_t)(controller_t *controller,
                                          epoch_t epoch);

/// @brief Initialize the controller and ensure resources are ready for use.
/// @memberof controller_t
/// @param controller pointer to the controller_t structure
/// @pre controller must be a valid pointer to a controller_t structure
/// @return @ref CONTROLLER_RESULT_OK if the controller was initialized \n
///         @ref CONTROLLER_RESULT_ERROR_INIT_FAILED if the controller could
///         not be initialized \n
///         @ref CONTROLLER_RESULT_ERROR_UNKNOWN if an error occurred
controller_result_t controller_init(controller_t *controller);

/// @brief Deinitialize the controller and ensure resource cleanup before exit
/// @memberof controller_t
/// @param controller pointer to the controller_t structure
/// @pre controller must be a valid pointer to a controller_t structure
/// @pre controller must have been initialized
/// @return @ref CONTROLLER_RESULT_OK if the controller was deinitialized \n
///         @ref CONTROLLER_RESULT_ERROR_DEINIT_FAILED if the controller could
///         not be deinitialized \n
///         @ref CONTROLLER_RESULT_ERROR_UNKNOWN if an error occurred
controller_result_t controller_deinit(controller_t *controller);

/// @brief Step the controller by going through the handlers we expect
/// @memberof controller_t
/// @param controller pointer to the controller_t structure
/// @pre controller must be a valid pointer to a controller_t structure
/// @pre controller must have been initialized
/// @return @ref CONTROLLER_RESULT_OK if the controller was stepped \n
///         @ref CONTROLLER_RESULT_ERROR_STEP_FAILED if the controller was
///         unable to perform a full epoch round \n
///         @ref CONTROLLER_RESULT_ERROR_UNKNOWN if an error occurred
controller_result_t controller_step(controller_t *controller);
#endif
/// @}
