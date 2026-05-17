#include "integrations/ahrs.h"
#include <stddef.h>
#include <stdint.h>

#define AHRS_PI_4 (0.7853981633974483)
#define AHRS_PI_3_4 (2.356194490192345)
#define AHRS_ATAN2_A (0.1963)
#define AHRS_ATAN2_B (0.9817)
#define AHRS_PI (3.14159265358979323846)
#define AHRS_Q15_ONE (32768U)
#define AHRS_Q15_MAX (32767U)

ahrs_t g_ahrs = {
    .config =
        {
            .ema_alpha = 0.1f,
        },
};

_Static_assert(sizeof(ahrs_public_t) == 64U,
               "ahrs_public_t wire size mismatch — sync Go ahrs_public_v1.go");
_Static_assert(offsetof(ahrs_public_t, heading) == 8U,
               "ahrs_public_t heading offset — sync mmap decoder");
_Static_assert(offsetof(ahrs_public_t, yaw) == 32U,
               "ahrs_public_t yaw offset — sync mmap decoder");
_Static_assert(offsetof(ahrs_public_t, rotation_rate_z) == 56U,
               "ahrs_public_t rotation_rate_z offset — sync mmap decoder");

static int16_t mag_apply_axis_scale(int32_t centered, int16_t num,
                                    int16_t den) {
  int32_t scaled;

  if (den == 0) {
    scaled = centered;
  } else {
    scaled = (centered * (int32_t)num) / (int32_t)den;
  }
  if (scaled < (int32_t)MAG_MEAS_MIN_DEC) {
    scaled = (int32_t)MAG_MEAS_MIN_DEC;
  } else if (scaled > (int32_t)MAG_MEAS_MAX_DEC) {
    scaled = (int32_t)MAG_MEAS_MAX_DEC;
  }
  return (int16_t)scaled;
}

static void ahrs_mag_corrected(const magnetometer_value_t *raw,
                               int16_t out[3]) {
  const magnetometer_calibration_t *c = &g_ahrs.magnetometer_calibration;
  const int32_t vx = (int32_t)raw->magnet_x - (int32_t)c->bias[0];
  const int32_t vy = (int32_t)raw->magnet_y - (int32_t)c->bias[1];
  const int32_t vz = (int32_t)raw->magnet_z - (int32_t)c->bias[2];

  out[0] = mag_apply_axis_scale(vx, c->scale_num[0], c->scale_den[0]);
  out[1] = mag_apply_axis_scale(vy, c->scale_num[1], c->scale_den[1]);
  out[2] = mag_apply_axis_scale(vz, c->scale_num[2], c->scale_den[2]);
}

static int16_t clamp_i16_from_i32(int32_t v) {
  int16_t out;
  if (v > 32767) {
    out = (int16_t)32767;
  } else if (v < -32768) {
    out = (int16_t)-32768;
  } else {
    out = (int16_t)v;
  }
  return out;
}

static uint32_t ahrs_alpha_q15(void) {
  const float a = g_ahrs.config.ema_alpha;
  uint32_t q;

  if (a <= 0.0f) {
    q = 1U;
  } else if (a >= 1.0f) {
    q = AHRS_Q15_MAX;
  } else {
    q = (uint32_t)(a * (float)AHRS_Q15_ONE + 0.5f);
    if (q < 1U) {
      q = 1U;
    } else if (q > AHRS_Q15_MAX) {
      q = AHRS_Q15_MAX;
    }
  }
  return q;
}

static int16_t ema_blend_int16(int16_t cur, int16_t raw, uint32_t alpha_q15) {
  const int64_t c = (int64_t)cur;
  const int64_t r = (int64_t)raw;
  const int64_t num =
      r * (int64_t)alpha_q15 + c * ((int64_t)AHRS_Q15_ONE - (int64_t)alpha_q15);
  const int32_t blended = (int32_t)(num >> 15);
  return clamp_i16_from_i32(blended);
}

static double ahrs_fabs(double v) {
  double out = v;
  if (out < 0.0) {
    out = -out;
  }
  return out;
}

/// Fast atan2 (radians) without libm; used for heading from smoothed mag XY.
/// @brief Fast atan2 (radians) without libm; used for heading from smoothed mag
/// XY.
/// @param y Y coordinate
/// @param x X coordinate
/// @pre X and Y are not both zero
/// @return Angle in radians
static double ahrs_atan2_rad(double y, double x) {
  double abs_y;
  double angle_rad;
  double r;
  abs_y = ahrs_fabs(y);
  if (x >= 0.0) {
    r = (x - abs_y) / (x + abs_y);
    angle_rad = AHRS_PI_4 + (AHRS_ATAN2_A * r * r - AHRS_ATAN2_B) * r;
  } else {
    r = (x + abs_y) / (abs_y - x);
    angle_rad = AHRS_PI_3_4 + (AHRS_ATAN2_A * r * r - AHRS_ATAN2_B) * r;
  }
  if (y < 0.0) {
    angle_rad = -angle_rad;
  }
  return angle_rad;
}

static double ahrs_normalize_heading_deg(double deg) {
  double out = deg;
  if (out < 0.0) {
    out += 360.0;
  }
  if (out >= 360.0) {
    out -= 360.0;
  }
  return out;
}

ahrs_result_t ahrs_init(void) {
  ahrs_result_t result = AHRS_RESULT_OK;
  const float alpha = g_ahrs.config.ema_alpha;

  if ((alpha <= 0.0f) || (alpha >= 1.0f)) {
    result = AHRS_RESULT_ERROR_INVALID_CONFIG;
  }

  if (result == AHRS_RESULT_OK) {
    g_ahrs.imu_state = AHRS_STATE_INITIALIZING;
    g_ahrs.magnetometer_state = AHRS_STATE_INITIALIZING;
    g_ahrs.smoothed_imu_value.accel_x = 0;
    g_ahrs.smoothed_imu_value.accel_y = 0;
    g_ahrs.smoothed_imu_value.accel_z = 0;
    g_ahrs.smoothed_imu_value.gyro_x = 0;
    g_ahrs.smoothed_imu_value.gyro_y = 0;
    g_ahrs.smoothed_imu_value.gyro_z = 0;
    g_ahrs.smoothed_magnetometer_value.magnet_x = 0;
    g_ahrs.smoothed_magnetometer_value.magnet_y = 0;
    g_ahrs.smoothed_magnetometer_value.magnet_z = 0;
    g_ahrs.mag_cal_sample_count = 0U;
    g_ahrs.mag_cal_have_bounds = 0U;
  }
  return result;
}

ahrs_result_t ahrs_update_imu(const imu_value_t *imu_value) {
  ahrs_result_t result = AHRS_RESULT_OK;
  const uint32_t aq = ahrs_alpha_q15();

  if (imu_value == NULL) {
    result = AHRS_RESULT_ERROR_UPDATE_FAILED;
  } else if ((g_ahrs.imu_state == AHRS_STATE_FAILED) ||
             (g_ahrs.imu_state == AHRS_STATE_DISABLED)) {
    result = AHRS_RESULT_ERROR_UPDATE_FAILED;
  } else if (g_ahrs.imu_state == AHRS_STATE_INITIALIZING) {
    g_ahrs.smoothed_imu_value.accel_x = imu_value->accel_x;
    g_ahrs.smoothed_imu_value.accel_y = imu_value->accel_y;
    g_ahrs.smoothed_imu_value.accel_z = imu_value->accel_z;
    g_ahrs.smoothed_imu_value.gyro_x = imu_value->gyro_x;
    g_ahrs.smoothed_imu_value.gyro_y = imu_value->gyro_y;
    g_ahrs.smoothed_imu_value.gyro_z = imu_value->gyro_z;
    g_ahrs.imu_state = AHRS_STATE_ACTIVE;
  } else {
    const int32_t cx =
        (int32_t)imu_value->accel_x - g_ahrs.imu_calibration.bias[0];
    const int32_t cy =
        (int32_t)imu_value->accel_y - g_ahrs.imu_calibration.bias[1];
    const int32_t cz =
        (int32_t)imu_value->accel_z - g_ahrs.imu_calibration.bias[2];

    g_ahrs.smoothed_imu_value.accel_x = ema_blend_int16(
        g_ahrs.smoothed_imu_value.accel_x, clamp_i16_from_i32(cx), aq);
    g_ahrs.smoothed_imu_value.accel_y = ema_blend_int16(
        g_ahrs.smoothed_imu_value.accel_y, clamp_i16_from_i32(cy), aq);
    g_ahrs.smoothed_imu_value.accel_z = ema_blend_int16(
        g_ahrs.smoothed_imu_value.accel_z, clamp_i16_from_i32(cz), aq);
    g_ahrs.smoothed_imu_value.gyro_x = ema_blend_int16(
        g_ahrs.smoothed_imu_value.gyro_x, imu_value->gyro_x, aq);
    g_ahrs.smoothed_imu_value.gyro_y = ema_blend_int16(
        g_ahrs.smoothed_imu_value.gyro_y, imu_value->gyro_y, aq);
    g_ahrs.smoothed_imu_value.gyro_z = ema_blend_int16(
        g_ahrs.smoothed_imu_value.gyro_z, imu_value->gyro_z, aq);

    if (g_ahrs.imu_state == AHRS_STATE_CALIBRATING) {
      ahrs_calibration_status_t status = ahrs_calibrate_imu();
      if (status == AHRS_CALIBRATION_RESULT_COMPLETED) {
        g_ahrs.imu_state = AHRS_STATE_ACTIVE;
      } else if (status == AHRS_CALIBRATION_RESULT_FAILED) {
        g_ahrs.imu_state = AHRS_STATE_FAILED;
      }
    }
  }
  return result;
}

ahrs_result_t
ahrs_update_magnetometer(const magnetometer_value_t *magnetometer_value) {
  ahrs_result_t result = AHRS_RESULT_OK;
  const uint32_t aq = ahrs_alpha_q15();

  if (magnetometer_value == NULL) {
    result = AHRS_RESULT_ERROR_UPDATE_FAILED;
  } else if ((g_ahrs.magnetometer_state == AHRS_STATE_FAILED) ||
             (g_ahrs.magnetometer_state == AHRS_STATE_DISABLED)) {
    result = AHRS_RESULT_ERROR_UPDATE_FAILED;
  } else if (g_ahrs.magnetometer_state == AHRS_STATE_CALIBRATING) {
    ahrs_calibration_status_t status =
        ahrs_calibrate_compass(magnetometer_value);
    if (status == AHRS_CALIBRATION_RESULT_COMPLETED) {
      g_ahrs.magnetometer_state = AHRS_STATE_ACTIVE;
    } else if (status == AHRS_CALIBRATION_RESULT_FAILED) {
      g_ahrs.magnetometer_state = AHRS_STATE_FAILED;
    }
  } else if (g_ahrs.magnetometer_state == AHRS_STATE_INITIALIZING) {
    int16_t c0[3];
    ahrs_mag_corrected(magnetometer_value, c0);
    g_ahrs.smoothed_magnetometer_value.magnet_x = c0[0];
    g_ahrs.smoothed_magnetometer_value.magnet_y = c0[1];
    g_ahrs.smoothed_magnetometer_value.magnet_z = c0[2];
    g_ahrs.magnetometer_state = AHRS_STATE_ACTIVE;
  } else {
    int16_t corr[3];
    ahrs_mag_corrected(magnetometer_value, corr);
    g_ahrs.smoothed_magnetometer_value.magnet_x = ema_blend_int16(
        g_ahrs.smoothed_magnetometer_value.magnet_x, corr[0], aq);
    g_ahrs.smoothed_magnetometer_value.magnet_y = ema_blend_int16(
        g_ahrs.smoothed_magnetometer_value.magnet_y, corr[1], aq);
    g_ahrs.smoothed_magnetometer_value.magnet_z = ema_blend_int16(
        g_ahrs.smoothed_magnetometer_value.magnet_z, corr[2], aq);
  }
  return result;
}

ahrs_public_t ahrs_get_data(void) {
  ahrs_public_t out;

  out.imu_state = g_ahrs.imu_state;
  out.magnetometer_state = g_ahrs.magnetometer_state;
  out.heading = 0.0;
  out.pitch = 0.0;
  out.roll = 0.0;
  out.yaw = 0.0;
  out.rotation_rate_x = 0.0;
  out.rotation_rate_y = 0.0;
  out.rotation_rate_z = 0.0;

  {
    int16_t mx = g_ahrs.smoothed_magnetometer_value.magnet_x;
    int16_t my = g_ahrs.smoothed_magnetometer_value.magnet_y;

    if ((g_ahrs.magnetometer_state == AHRS_STATE_ACTIVE) &&
        ((mx != 0) || (my != 0))) {
      double rad = ahrs_atan2_rad((double)my, (double)mx);
      double heading_deg = rad * (180.0 / AHRS_PI);
      heading_deg = ahrs_normalize_heading_deg(heading_deg);
      out.heading = heading_deg;
      out.yaw = heading_deg;
    }

    // calculate pitch and roll
    // TODO: implement this and fix all the magic numbers
  }

  return out;
}
