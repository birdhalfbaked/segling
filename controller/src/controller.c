#include "controller/controller.h"
#include "controller/comms.h"
#include "controller/storage.h"
#include <time.h>

/// Integration Configurations

/// AHRS
#ifndef AHRS_ENABLED
#define AHRS_ENABLED 0
#else
#include "integrations/ahrs.h"
#define AHRS_ENABLED 1
#endif

/// GPS
#ifndef GPS_ENABLED
#define GPS_ENABLED 0
#else
#include "integrations/gps.h"
#define GPS_ENABLED 1
#endif

/// Barometer
#ifndef BAROMETER_ENABLED
#define BAROMETER_ENABLED 0
#else
#include "integrations/barometer.h"
#define BAROMETER_ENABLED 1
#endif

/// Rudder
#ifndef RUDDER_ENABLED
#define RUDDER_ENABLED 0
#else
#include "integrations/rudder.h"
#define RUDDER_ENABLED 1
#endif

/// Autopilot
#ifndef AUTOPILOT_ENABLED
#define AUTOPILOT_ENABLED 0
#else
#include "integrations/autopilot.h"
#define AUTOPILOT_ENABLED 1
#endif

/// Interface
#ifndef INTERFACE_ENABLED
#define INTERFACE_ENABLED 0
#else
#include "integrations/interface.h"
#define INTERFACE_ENABLED 1
#endif

static controller_slot_handler_t g_slot_handlers[SLOT_COUNT] = {
#if AHRS_ENABLED
    [SLOT_ID_AHRS] = ahrs_step,
#endif
#if GPS_ENABLED
    [SLOT_ID_GPS] = gps_step,
#endif
#if BAROMETER_ENABLED
    [SLOT_ID_BAROMETER] = barometer_step,
#endif
#if RUDDER_ENABLED
    [SLOT_ID_RUDDER] = rudder_step,
#endif
#if AUTOPILOT_ENABLED
    [SLOT_ID_AUTOPILOT] = autopilot_step,
#endif
#if INTERFACE_ENABLED
    [SLOT_ID_INTERFACE] = interface_step,
#endif
};

controller_config_t controller_config = {
    .log_level = LOG_LEVEL_INFO, // default to info
};

controller_result_t controller_init(controller_t *controller) {
  logging_init();
  controller_result_t result = CONTROLLER_RESULT_OK;
  comms_result_t comms_result = comms_init(&controller->comms);
  if (comms_result != COMMS_RESULT_OK) {
    result = CONTROLLER_RESULT_ERROR_INIT_FAILED;
  }
  if (result == CONTROLLER_RESULT_OK) {
    storage_result_t storage_result = storage_init(&controller->storage);
    if (storage_result != STORAGE_RESULT_OK) {
      result = CONTROLLER_RESULT_ERROR_INIT_FAILED;
    }
  }
  if (result == CONTROLLER_RESULT_OK) {
    controller->epoch = 0;
  }
  return result;
}
controller_result_t controller_deinit(controller_t *controller) {
  controller_result_t result = CONTROLLER_RESULT_OK;
  comms_result_t comms_result = comms_deinit(&controller->comms);
  if (comms_result != COMMS_RESULT_OK) {
    result = CONTROLLER_RESULT_ERROR_DEINIT_FAILED;
  }
  if (result == CONTROLLER_RESULT_OK) {
    storage_result_t storage_result = storage_deinit(&controller->storage);
    if (storage_result != STORAGE_RESULT_OK) {
      result = CONTROLLER_RESULT_ERROR_DEINIT_FAILED;
    }
  }
  return result;
}

uint64_t get_abs_time_diff_us(struct timespec *expected) {
  struct timespec current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  int64_t diff_us = (current_time.tv_sec - expected->tv_sec) * 1000000 +
                    (current_time.tv_nsec - expected->tv_nsec) / 1000;
  if (diff_us < 0)
    diff_us = -diff_us;
  return (uint64_t)diff_us;
}

controller_result_t controller_step(controller_t *controller) {
  controller->epoch++;
  controller->current_slot = 0;
  controller_result_t result = CONTROLLER_RESULT_OK;
  for (int i = 0; i < SLOT_COUNT; i++) {
    // evaluate the current time and start slot time for jitter threshold
    // adherence
    uint64_t detected_jitter_us =
        get_abs_time_diff_us(&controller->next_slot_start_time);

    if (detected_jitter_us <= MAX_SLOT_JITTER_US) {
      if (g_slot_handlers[i] != NULL) {
        g_slot_handlers[i](controller, controller->epoch);
      }
    } else {
      log_messagef(controller->epoch, controller->current_slot, LOG_LEVEL_ERROR,
                   "Slot jitter exceeded: %lu us", detected_jitter_us);
    }

    // set next slot time
    controller->next_slot_start_time = (struct timespec){
        .tv_sec = controller->next_slot_start_time.tv_sec,
        .tv_nsec =
            controller->next_slot_start_time.tv_nsec + (SLOT_TIME_US * 1000),
    };
    if (controller->next_slot_start_time.tv_nsec >= 1000000000) {
      controller->next_slot_start_time.tv_sec++;
      controller->next_slot_start_time.tv_nsec %= 1000000000;
    }
    // TODO: write controller state to storage
    if (i < SLOT_COUNT - 1) {
      if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                          &controller->next_slot_start_time, NULL) != 0) {
        result = CONTROLLER_RESULT_ERROR_UNKNOWN;
        break;
      }
      controller->current_slot++;
    }
  }

  // finish loop and then do final sleep
  if (result == CONTROLLER_RESULT_OK &&
      clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                      &controller->next_slot_start_time, NULL) != 0) {
    result = CONTROLLER_RESULT_ERROR_UNKNOWN;
  }
  return result;
}