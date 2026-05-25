/// @defgroup INTERFACE Interface module
/// @ingroup INTEGRATIONS
/// @{
#ifndef INTEGRATIONS_INTERFACE_H
#define INTEGRATIONS_INTERFACE_H

#include "controller/controller.h"
#include <stdint.h>

void interface_step(controller_t *controller, epoch_t epoch);

#endif
/// @}
