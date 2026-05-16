#ifndef INTEGRATIONS_BAROMETER_H
#define INTEGRATIONS_BAROMETER_H

#include "controller/controller.h"
#include <stdint.h>

void barometer_step(controller_t *controller, epoch_t epoch);

#endif