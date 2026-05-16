#ifndef INTEGRATIONS_GPS_H
#define INTEGRATIONS_GPS_H

#include "controller/controller.h"
#include <stdint.h>

void gps_step(controller_t *controller, epoch_t epoch);

#endif