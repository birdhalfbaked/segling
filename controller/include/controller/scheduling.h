/// @defgroup SCHEDULING Scheduling module
/// @ingroup CORE
/// @{
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
  SLOT_ID_AHRS = 0,
  SLOT_ID_GPS = 1,
  SLOT_ID_BAROMETER = 2,
  SLOT_ID_RUDDER = 3,
  SLOT_ID_AUTOPILOT = 4,
  SLOT_ID_INTERFACE = 5,
} slot_id_t;

typedef enum {
  SCHEDULING_RESULT_OK = 0,
  SCHEDULING_RESULT_ERROR_UNKNOWN = -1,
  SCHEDULING_RESULT_ERROR_INVALID_EPOCH = -2,
} scheduling_result_t;
/// @}
#endif
/// @}
