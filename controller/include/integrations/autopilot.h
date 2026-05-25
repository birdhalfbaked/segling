/// @defgroup AUTOPILOT Autopilot module
/// @ingroup INTEGRATIONS
/// @{
#ifndef INTEGRATIONS_AUTOPILOT_H
#define INTEGRATIONS_AUTOPILOT_H

#include "controller/controller.h"
#include <stdint.h>

void autopilot_step(controller_t *controller, epoch_t epoch);

#endif
/// @}
