#ifndef INTEGRATIONS_RUDDER_H
#define INTEGRATIONS_RUDDER_H

#include "controller/controller.h"
#include <stdint.h>

void rudder_step(controller_t *controller, epoch_t epoch);

#endif