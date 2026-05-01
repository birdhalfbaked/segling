#include "ahrs.h"
#include <stdbool.h>
#include <stddef.h>

bool imu_calibration_is_valid(imu_calibration_t *imu_calibration) {
  return imu_calibration->bias[0] != 0.0f && imu_calibration->bias[1] != 0.0f &&
         imu_calibration->bias[2] != 0.0f;
}

bool magnetometer_calibration_is_valid(
    magnetometer_calibration_t *magnetometer_calibration) {
  return magnetometer_calibration->bias[0] != 0.0f &&
         magnetometer_calibration->bias[1] != 0.0f &&
         magnetometer_calibration->bias[2] != 0.0f;
}

ahrs_result_t ahrs_init(ahrs_t *ahrs) {
  ahrs_result_t result = AHRS_RESULT_OK;
  if (ahrs == NULL) {
    result = AHRS_RESULT_ERROR_INVALID_CONFIG;
  }

  if (result == AHRS_RESULT_OK) {
    if (ahrs->config->ema_alpha <= 0.0f || ahrs->config->ema_alpha >= 1.0f ||
        !imu_calibration_is_valid(&ahrs->imu_calibration) ||
        !magnetometer_calibration_is_valid(&ahrs->magnetometer_calibration)) {
      result = AHRS_RESULT_ERROR_INVALID_CONFIG;
    }
  }

  if (result == AHRS_RESULT_OK) {
    ahrs->imu_state = AHRS_STATE_INITIALIZING;
    ahrs->magnetometer_state = AHRS_STATE_INITIALIZING;
  }
  return result;
}

ahrs_result_t ahrs_calibrate_compass(ahrs_t *ahrs) {
  ahrs_result_t result = AHRS_RESULT_OK;
  if (ahrs == NULL) {
    result = AHRS_RESULT_ERROR_INVALID_CONFIG;
  }

  if (result == AHRS_RESULT_OK) {
    ahrs->magnetometer_state = AHRS_STATE_CALIBRATING;
  }
  return AHRS_RESULT_OK;
}

ahrs_result_t ahrs_calibrate_imu(ahrs_t *ahrs) {
  ahrs_result_t result = AHRS_RESULT_OK;
  if (ahrs == NULL) {
    result = AHRS_RESULT_ERROR_INVALID_CONFIG;
  }

  if (result == AHRS_RESULT_OK) {
    ahrs->imu_state = AHRS_STATE_CALIBRATING;
  }
  return result;
}
