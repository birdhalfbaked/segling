#ifndef SCHEDULING_H
#define SCHEDULING_H

#include <stdint.h>

/// @defgroup SCHEDULING_types SCHEDULING types
/// @{

#define SLOT_TIME_US 15000
#define MAX_SLOT_JITTER_US 500
/// Keep this just since we may add more later and I am lazy
#define SLOT_COUNT 16

typedef uint64_t epoch_t;

typedef enum {
  FILE_ID_AHRS = 0,
  FILE_ID_GPS = 1,
  FILE_ID_BAROMETER = 2,
  FILE_ID_RUDDER = 3,
  FILE_ID_AUTOPILOT = 4,
  FILE_ID_INTERFACE = 5,
} slot_id_t;

/// @}

#endif