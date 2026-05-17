#include "integrations/ahrs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool mag_cal_try_finish(void) {
  int32_t span[AHRS_AXIS_COUNT];
  int32_t avg_span;
  bool ok = true;

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

  if (ok) {

    avg_span = 0;
    for (i = 0U; i < AHRS_AXIS_COUNT; i++) {
      avg_span += span[i];
    }
    avg_span /= AHRS_AXIS_COUNT;
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
  }
  return ok;
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

void ahrs_reset_compass_calibration(void) {
  g_ahrs.mag_cal_sample_count = 0U;
  g_ahrs.mag_cal_have_bounds = 0U;
  g_ahrs.magnetometer_calibration.bias[0] = 0;
  g_ahrs.magnetometer_calibration.bias[1] = 0;
  g_ahrs.magnetometer_calibration.bias[2] = 0;
  g_ahrs.magnetometer_calibration.scale_num[0] = 1;
  g_ahrs.magnetometer_calibration.scale_num[1] = 1;
  g_ahrs.magnetometer_calibration.scale_num[2] = 1;
  g_ahrs.magnetometer_calibration.scale_den[0] = 1;
  g_ahrs.magnetometer_calibration.scale_den[1] = 1;
  g_ahrs.magnetometer_calibration.scale_den[2] = 1;
}

ahrs_calibration_status_t
ahrs_calibrate_compass(const magnetometer_value_t *magnetometer_value) {
  ahrs_calibration_status_t status = AHRS_CALIBRATION_RESULT_IN_PROGRESS;

  if (magnetometer_value == NULL) {
    if (g_ahrs.magnetometer_state == AHRS_STATE_ACTIVE) {
      ahrs_reset_compass_calibration();
      g_ahrs.magnetometer_state = AHRS_STATE_CALIBRATING;
    } else {
      status = AHRS_CALIBRATION_RESULT_FAILED;
    }
  } else if (g_ahrs.magnetometer_state != AHRS_STATE_CALIBRATING) {
    status = AHRS_CALIBRATION_RESULT_FAILED;
  } else {
    mag_cal_note_sample(magnetometer_value);

    if (g_ahrs.mag_cal_sample_count >= AHRS_MAG_CAL_MAX_SAMPLES) {
      status = AHRS_CALIBRATION_RESULT_FAILED;
    } else if ((g_ahrs.mag_cal_sample_count >= AHRS_MAG_CAL_MIN_SAMPLES) &&
               mag_cal_try_finish()) {
      status = AHRS_CALIBRATION_RESULT_COMPLETED;
    }
  }
  return status;
}

void ahrs_reset_imu_calibration(void) {
  g_ahrs.imu_calibration.bias[0] = 0;
  g_ahrs.imu_calibration.bias[1] = 0;
  g_ahrs.imu_calibration.bias[2] = 0;
}

ahrs_calibration_status_t ahrs_calibrate_imu(void) {
  if (g_ahrs.imu_state == AHRS_STATE_ACTIVE) {
    g_ahrs.imu_state = AHRS_STATE_CALIBRATING;
    return AHRS_CALIBRATION_RESULT_IN_PROGRESS;
  }
  if (g_ahrs.imu_state == AHRS_STATE_CALIBRATING) {
    g_ahrs.imu_calibration.bias[0] = (int32_t)g_ahrs.smoothed_imu_value.accel_x;
    g_ahrs.imu_calibration.bias[1] = (int32_t)g_ahrs.smoothed_imu_value.accel_y;
    g_ahrs.imu_calibration.bias[2] = (int32_t)g_ahrs.smoothed_imu_value.accel_z;
    return AHRS_CALIBRATION_RESULT_COMPLETED;
  }
  return AHRS_CALIBRATION_RESULT_FAILED;
}
