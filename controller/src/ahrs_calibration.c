#include "integrations/ahrs.h"
#include <stddef.h>
#include <stdint.h>

#define AHRS_AXIS_COUNT (3U)

static uint8_t mag_cal_try_finish(void) {
  int32_t span[3];
  int32_t avg_span;
  uint8_t ok = 1U;
  uint8_t i;

  for (i = 0U; i < AHRS_AXIS_COUNT; i++) {
    span[i] = (int32_t)g_ahrs.mag_cal_max[i] - (int32_t)g_ahrs.mag_cal_min[i];
    if (span[i] < (int32_t)AHRS_MAG_CAL_MIN_SPAN) {
      ok = 0U;
    }
    if (span[i] < 1) {
      span[i] = 1;
    }
  }

  if (ok == 0U) {
    return 0U;
  }

  avg_span = (span[0] + span[1] + span[2]) / 3;
  if (avg_span < 1) {
    avg_span = 1;
  }

  for (i = 0U; i < AHRS_AXIS_COUNT; i++) {
    const int32_t mid =
        ((int32_t)g_ahrs.mag_cal_min[i] + (int32_t)g_ahrs.mag_cal_max[i]) / 2;
    g_ahrs.magnetometer_calibration.bias[i] = (int16_t)mid;
    g_ahrs.magnetometer_calibration.scale_num[i] = (int16_t)avg_span;
    g_ahrs.magnetometer_calibration.scale_den[i] = (int16_t)span[i];
  }

  return 1U;
}

static void
mag_cal_note_sample(const magnetometer_value_t *magnetometer_value) {
  const int16_t ax = magnetometer_value->magnet_x;
  const int16_t ay = magnetometer_value->magnet_y;
  const int16_t az = magnetometer_value->magnet_z;

  if (g_ahrs.mag_cal_have_bounds == 0U) {
    g_ahrs.mag_cal_min[0] = ax;
    g_ahrs.mag_cal_max[0] = ax;
    g_ahrs.mag_cal_min[1] = ay;
    g_ahrs.mag_cal_max[1] = ay;
    g_ahrs.mag_cal_min[2] = az;
    g_ahrs.mag_cal_max[2] = az;
    g_ahrs.mag_cal_have_bounds = 1U;
  } else {
    if (ax < g_ahrs.mag_cal_min[0]) {
      g_ahrs.mag_cal_min[0] = ax;
    }
    if (ax > g_ahrs.mag_cal_max[0]) {
      g_ahrs.mag_cal_max[0] = ax;
    }
    if (ay < g_ahrs.mag_cal_min[1]) {
      g_ahrs.mag_cal_min[1] = ay;
    }
    if (ay > g_ahrs.mag_cal_max[1]) {
      g_ahrs.mag_cal_max[1] = ay;
    }
    if (az < g_ahrs.mag_cal_min[2]) {
      g_ahrs.mag_cal_min[2] = az;
    }
    if (az > g_ahrs.mag_cal_max[2]) {
      g_ahrs.mag_cal_max[2] = az;
    }
  }

  if (g_ahrs.mag_cal_sample_count < UINT16_MAX) {
    g_ahrs.mag_cal_sample_count++;
  }
}

static void mag_cal_reset_collection(void) {
  g_ahrs.mag_cal_sample_count = 0U;
  g_ahrs.mag_cal_have_bounds = 0U;
}

ahrs_result_t
ahrs_calibrate_compass(const magnetometer_value_t *magnetometer_value) {
  ahrs_result_t result = AHRS_RESULT_OK;

  if (g_ahrs.magnetometer_state == AHRS_STATE_ACTIVE) {
    mag_cal_reset_collection();
    g_ahrs.magnetometer_state = AHRS_STATE_CALIBRATING;
  } else if (g_ahrs.magnetometer_state == AHRS_STATE_CALIBRATING) {
    if (magnetometer_value == NULL) {
      result = AHRS_RESULT_ERROR_UPDATE_FAILED;
    } else {
      mag_cal_note_sample(magnetometer_value);

      if (g_ahrs.mag_cal_sample_count >= AHRS_MAG_CAL_MAX_SAMPLES) {
        g_ahrs.magnetometer_state = AHRS_STATE_FAILED;
      } else if ((g_ahrs.mag_cal_sample_count >= AHRS_MAG_CAL_MIN_SAMPLES) &&
                 (mag_cal_try_finish() != 0U)) {
        g_ahrs.magnetometer_state = AHRS_STATE_ACTIVE;
      } else {
        /* Still collecting calibration samples. */
      }
    }
  } else {
    result = AHRS_RESULT_ERROR_CALIBRATION_FAILED;
  }
  return result;
}

ahrs_result_t ahrs_calibrate_imu(void) {
  ahrs_result_t result = AHRS_RESULT_OK;

  if (g_ahrs.imu_state == AHRS_STATE_ACTIVE) {
    g_ahrs.imu_state = AHRS_STATE_CALIBRATING;
  } else if (g_ahrs.imu_state == AHRS_STATE_CALIBRATING) {
    g_ahrs.imu_calibration.bias[0] = (int32_t)g_ahrs.smoothed_imu_value.accel_x;
    g_ahrs.imu_calibration.bias[1] = (int32_t)g_ahrs.smoothed_imu_value.accel_y;
    g_ahrs.imu_calibration.bias[2] = (int32_t)g_ahrs.smoothed_imu_value.accel_z;
    g_ahrs.imu_state = AHRS_STATE_ACTIVE;
  } else {
    result = AHRS_RESULT_ERROR_CALIBRATION_FAILED;
  }
  return result;
}
