#include "ahrs.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

_Static_assert(sizeof(ahrs_public_t) == 64U,
               "ahrs_public_t wire size mismatch — sync Go ahrs_public_v1.go");
_Static_assert(offsetof(ahrs_public_t, heading) == 8U,
               "ahrs_public_t heading offset — sync mmap decoder");
_Static_assert(offsetof(ahrs_public_t, yaw) == 32U,
               "ahrs_public_t yaw offset — sync mmap decoder");
_Static_assert(offsetof(ahrs_public_t, rotation_rate_z) == 56U,
               "ahrs_public_t rotation_rate_z offset — sync mmap decoder");

static int16_t clamp_i16_from_i32(int32_t v) {
  if (v > 32767) {
    return (int16_t)32767;
  }
  if (v < -32768) {
    return (int16_t)-32768;
  }
  return (int16_t)v;
}

/// Convert config ema_alpha \in (0,1) to Q15 fixed point over 32768
/// denominator.
static uint32_t ahrs_alpha_q15(const ahrs_config_t *cfg) {
  float a = cfg->ema_alpha;
  if (a <= 0.0f) {
    return 1u;
  }
  if (a >= 1.0f) {
    return 32767u;
  }
  uint32_t q = (uint32_t)(a * 32768.0f + 0.5f);
  if (q < 1u) {
    return 1u;
  }
  if (q > 32767u) {
    return 32767u;
  }
  return q;
}

/// EMA on int16 samples: blended = (alpha*raw + (1-alpha)*cur) with alpha in
/// Q15.
static void ema_blend_int16(int16_t *cur, int16_t raw, uint32_t alpha_q15) {
  int64_t c = *cur;
  int64_t r = raw;
  int64_t num =
      r * (int64_t)alpha_q15 + c * ((int64_t)32768 - (int64_t)alpha_q15);
  int32_t blended = (int32_t)(num >> 15);
  *cur = clamp_i16_from_i32(blended);
}

static int16_t mag_apply_axis_scale(int32_t centered, int16_t num,
                                    int16_t den) {
  int32_t scaled;
  if (den == 0) {
    scaled = centered;
  } else {
    scaled = (centered * (int32_t)num) / (int32_t)den;
  }
  if (scaled < (int32_t)MAG_MEAS_MIN_DEC) {
    return MAG_MEAS_MIN_DEC;
  }
  if (scaled > (int32_t)MAG_MEAS_MAX_DEC) {
    return MAG_MEAS_MAX_DEC;
  }
  return (int16_t)scaled;
}

static void mag_corrected(const ahrs_t *ahrs, const magnetometer_value_t *raw,
                          int16_t out[3]) {
  const magnetometer_calibration_t *c = &ahrs->magnetometer_calibration;
  int32_t vx = (int32_t)raw->magnet_x - (int32_t)c->bias[0];
  int32_t vy = (int32_t)raw->magnet_y - (int32_t)c->bias[1];
  int32_t vz = (int32_t)raw->magnet_z - (int32_t)c->bias[2];
  out[0] = mag_apply_axis_scale(vx, c->scale_num[0], c->scale_den[0]);
  out[1] = mag_apply_axis_scale(vy, c->scale_num[1], c->scale_den[1]);
  out[2] = mag_apply_axis_scale(vz, c->scale_num[2], c->scale_den[2]);
}

static bool mag_cal_try_finish(ahrs_t *ahrs) {
  int32_t span[3];
  for (int i = 0; i < 3; i++) {
    span[i] = (int32_t)ahrs->mag_cal_max[i] - (int32_t)ahrs->mag_cal_min[i];
    if (span[i] < (int32_t)AHRS_MAG_CAL_MIN_SPAN) {
      return false;
    }
    if (span[i] < 1) {
      span[i] = 1;
    }
  }

  int32_t avg_span = (span[0] + span[1] + span[2]) / 3;
  if (avg_span < 1) {
    avg_span = 1;
  }

  for (int i = 0; i < 3; i++) {
    int32_t mid =
        ((int32_t)ahrs->mag_cal_min[i] + (int32_t)ahrs->mag_cal_max[i]) / 2;
    ahrs->magnetometer_calibration.bias[i] = (int16_t)mid;
    ahrs->magnetometer_calibration.scale_num[i] = (int16_t)avg_span;
    ahrs->magnetometer_calibration.scale_den[i] = (int16_t)span[i];
  }

  return true;
}

static void
mag_cal_note_sample(ahrs_t *ahrs,
                    const magnetometer_value_t *magnetometer_value) {
  const int16_t ax = magnetometer_value->magnet_x;
  const int16_t ay = magnetometer_value->magnet_y;
  const int16_t az = magnetometer_value->magnet_z;

  if (!ahrs->mag_cal_have_bounds) {
    ahrs->mag_cal_min[0] = ahrs->mag_cal_max[0] = ax;
    ahrs->mag_cal_min[1] = ahrs->mag_cal_max[1] = ay;
    ahrs->mag_cal_min[2] = ahrs->mag_cal_max[2] = az;
    ahrs->mag_cal_have_bounds = 1;
  } else {
    if (ax < ahrs->mag_cal_min[0]) {
      ahrs->mag_cal_min[0] = ax;
    }
    if (ax > ahrs->mag_cal_max[0]) {
      ahrs->mag_cal_max[0] = ax;
    }
    if (ay < ahrs->mag_cal_min[1]) {
      ahrs->mag_cal_min[1] = ay;
    }
    if (ay > ahrs->mag_cal_max[1]) {
      ahrs->mag_cal_max[1] = ay;
    }
    if (az < ahrs->mag_cal_min[2]) {
      ahrs->mag_cal_min[2] = az;
    }
    if (az > ahrs->mag_cal_max[2]) {
      ahrs->mag_cal_max[2] = az;
    }
  }

  if (ahrs->mag_cal_sample_count < UINT16_MAX) {
    ahrs->mag_cal_sample_count++;
  }
}

ahrs_result_t ahrs_init(ahrs_t *ahrs) {
  ahrs_result_t result = AHRS_RESULT_OK;
  if (ahrs == NULL || ahrs->config == NULL) {
    result = AHRS_RESULT_ERROR_INVALID_CONFIG;
  }

  if (result == AHRS_RESULT_OK) {
    if (ahrs->config->ema_alpha <= 0.0f || ahrs->config->ema_alpha >= 1.0f) {
      result = AHRS_RESULT_ERROR_INVALID_CONFIG;
    }
  }

  if (result == AHRS_RESULT_OK) {
    ahrs->imu_state = AHRS_STATE_INITIALIZING;
    ahrs->magnetometer_state = AHRS_STATE_INITIALIZING;
    ahrs->smoothed_imu_value.accel_x = 0;
    ahrs->smoothed_imu_value.accel_y = 0;
    ahrs->smoothed_imu_value.accel_z = 0;
    ahrs->smoothed_imu_value.gyro_x = 0;
    ahrs->smoothed_imu_value.gyro_y = 0;
    ahrs->smoothed_imu_value.gyro_z = 0;
    ahrs->smoothed_magnetometer_value.magnet_x = 0;
    ahrs->smoothed_magnetometer_value.magnet_y = 0;
    ahrs->smoothed_magnetometer_value.magnet_z = 0;
    ahrs->mag_cal_sample_count = 0;
    ahrs->mag_cal_have_bounds = 0;
  }
  return result;
}

ahrs_result_t ahrs_calibrate_compass(ahrs_t *ahrs) {
  if (ahrs == NULL || ahrs->config == NULL) {
    return AHRS_RESULT_ERROR_INVALID_CONFIG;
  }
  if (ahrs->magnetometer_state != AHRS_STATE_ACTIVE) {
    return AHRS_RESULT_ERROR_CALIBRATION_FAILED;
  }
  ahrs->mag_cal_sample_count = 0;
  ahrs->mag_cal_have_bounds = 0;
  ahrs->magnetometer_state = AHRS_STATE_CALIBRATING;
  return AHRS_RESULT_OK;
}

ahrs_result_t ahrs_calibrate_imu(ahrs_t *ahrs) {
  if (ahrs == NULL || ahrs->config == NULL) {
    return AHRS_RESULT_ERROR_INVALID_CONFIG;
  }
  if (ahrs->imu_state != AHRS_STATE_ACTIVE) {
    return AHRS_RESULT_ERROR_CALIBRATION_FAILED;
  }
  ahrs->imu_state = AHRS_STATE_CALIBRATING;
  return AHRS_RESULT_OK;
}

ahrs_result_t ahrs_update_imu(ahrs_t *ahrs, imu_value_t *imu_value) {
  if (ahrs == NULL || imu_value == NULL || ahrs->config == NULL) {
    return AHRS_RESULT_ERROR_UPDATE_FAILED;
  }
  if (ahrs->imu_state == AHRS_STATE_FAILED ||
      ahrs->imu_state == AHRS_STATE_DISABLED) {
    return AHRS_RESULT_ERROR_UPDATE_FAILED;
  }

  uint32_t aq = ahrs_alpha_q15(ahrs->config);

  if (ahrs->imu_state == AHRS_STATE_INITIALIZING) {
    ahrs->smoothed_imu_value = *imu_value;
    ahrs->imu_state = AHRS_STATE_ACTIVE;
    return AHRS_RESULT_OK;
  }

  int32_t cx = (int32_t)imu_value->accel_x - ahrs->imu_calibration.bias[0];
  int32_t cy = (int32_t)imu_value->accel_y - ahrs->imu_calibration.bias[1];
  int32_t cz = (int32_t)imu_value->accel_z - ahrs->imu_calibration.bias[2];

  ema_blend_int16(&ahrs->smoothed_imu_value.accel_x, clamp_i16_from_i32(cx),
                  aq);
  ema_blend_int16(&ahrs->smoothed_imu_value.accel_y, clamp_i16_from_i32(cy),
                  aq);
  ema_blend_int16(&ahrs->smoothed_imu_value.accel_z, clamp_i16_from_i32(cz),
                  aq);

  ema_blend_int16(&ahrs->smoothed_imu_value.gyro_x, imu_value->gyro_x, aq);
  ema_blend_int16(&ahrs->smoothed_imu_value.gyro_y, imu_value->gyro_y, aq);
  ema_blend_int16(&ahrs->smoothed_imu_value.gyro_z, imu_value->gyro_z, aq);

  if (ahrs->imu_state == AHRS_STATE_CALIBRATING) {
    ahrs->imu_calibration.bias[0] = (int32_t)ahrs->smoothed_imu_value.accel_x;
    ahrs->imu_calibration.bias[1] = (int32_t)ahrs->smoothed_imu_value.accel_y;
    ahrs->imu_calibration.bias[2] = (int32_t)ahrs->smoothed_imu_value.accel_z;
    ahrs->imu_state = AHRS_STATE_ACTIVE;
  }

  return AHRS_RESULT_OK;
}

ahrs_result_t
ahrs_update_magnetometer(ahrs_t *ahrs,
                         magnetometer_value_t *magnetometer_value) {
  if (ahrs == NULL || magnetometer_value == NULL || ahrs->config == NULL) {
    return AHRS_RESULT_ERROR_UPDATE_FAILED;
  }
  if (ahrs->magnetometer_state == AHRS_STATE_FAILED ||
      ahrs->magnetometer_state == AHRS_STATE_DISABLED) {
    return AHRS_RESULT_ERROR_UPDATE_FAILED;
  }

  uint32_t aq = ahrs_alpha_q15(ahrs->config);

  if (ahrs->magnetometer_state == AHRS_STATE_CALIBRATING) {
    mag_cal_note_sample(ahrs, magnetometer_value);

    if (ahrs->mag_cal_sample_count >= AHRS_MAG_CAL_MAX_SAMPLES) {
      ahrs->magnetometer_state = AHRS_STATE_FAILED;
      return AHRS_RESULT_OK;
    }

    if (ahrs->mag_cal_sample_count >= AHRS_MAG_CAL_MIN_SAMPLES &&
        mag_cal_try_finish(ahrs)) {
      ahrs->magnetometer_state = AHRS_STATE_ACTIVE;
    } else {
      return AHRS_RESULT_OK;
    }
  }

  if (ahrs->magnetometer_state == AHRS_STATE_INITIALIZING) {
    int16_t c0[3];
    mag_corrected(ahrs, magnetometer_value, c0);
    ahrs->smoothed_magnetometer_value.magnet_x = c0[0];
    ahrs->smoothed_magnetometer_value.magnet_y = c0[1];
    ahrs->smoothed_magnetometer_value.magnet_z = c0[2];
    ahrs->magnetometer_state = AHRS_STATE_ACTIVE;
    return AHRS_RESULT_OK;
  }

  int16_t corr[3];
  mag_corrected(ahrs, magnetometer_value, corr);
  ema_blend_int16(&ahrs->smoothed_magnetometer_value.magnet_x, corr[0], aq);
  ema_blend_int16(&ahrs->smoothed_magnetometer_value.magnet_y, corr[1], aq);
  ema_blend_int16(&ahrs->smoothed_magnetometer_value.magnet_z, corr[2], aq);

  return AHRS_RESULT_OK;
}

ahrs_public_t ahrs_get_data(ahrs_t *ahrs) {
  ahrs_public_t out = {0};
  if (ahrs == NULL) {
    return out;
  }
  out.imu_state = ahrs->imu_state;
  out.magnetometer_state = ahrs->magnetometer_state;

  int16_t mx = ahrs->smoothed_magnetometer_value.magnet_x;
  int16_t my = ahrs->smoothed_magnetometer_value.magnet_y;
  int16_t mz = ahrs->smoothed_magnetometer_value.magnet_z;

  if (ahrs->magnetometer_state == AHRS_STATE_ACTIVE && (mx != 0 || my != 0)) {
    (void)mz;
    /* Smoothed mx,my are integers; heading is a derived angle — atan2 is FP. */
    double rad = atan2((double)my, (double)mx);
    out.heading = rad * (180.0 / M_PI);
    if (out.heading < 0.0) {
      out.heading += 360.0;
    }
    out.yaw = out.heading;
  }

  out.pitch = 0.0;
  out.roll = 0.0;
  out.rotation_rate_x = 0.0;
  out.rotation_rate_y = 0.0;
  out.rotation_rate_z = 0.0;
  return out;
}
