/// @defgroup RUDDER Rudder module
/// @ingroup INTEGRATIONS
/// @{
#ifndef INTEGRATIONS_RUDDER_H
#define INTEGRATIONS_RUDDER_H

#include "controller/controller.h"
#include <stdint.h>

/// @defgroup RUDDER_types RUDDER types
/// @brief Rudder actuator mode
/// @details Uses binary encoding for voltage to relay mappings
///  0b00: no actuation, restricted movement
///  0b01: Move the rudder actuator to the left
///  0b10: Move the rudder actuator to the right
///  0b11: no actuation, unrestricted movement voltage positive both relays
/// @{
typedef enum {
  /// @brief no actuation, restricted movement
  RUDDER_ACTUATOR_HOLD = 0x00,
  /// @brief Move the rudder actuator to the left
  RUDDER_ACTUATOR_LEFT = 0x01,
  /// @brief Move the rudder actuator to the right
  RUDDER_ACTUATOR_RIGHT = 0x02,
  /// @brief no actuation, unrestricted movement voltage positive both relays
  RUDDER_ACTUATOR_FREE = 0x03,
} rudder_actuator_mode_t;

/// @}
/// @defgroup RUDDER_data RUDDER data
/// @{
typedef struct {
  rudder_actuator_mode_t actuator_mode;
} rudder_t;

/// @}
extern rudder_t g_rudder;

/// @defgroup RUDDER_methods RUDDER methods
/// @{
void rudder_step(controller_t *controller, epoch_t epoch);
/// @}
#endif
/// @}
