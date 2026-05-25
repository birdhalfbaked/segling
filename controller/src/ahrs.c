#include "integrations/ahrs.h"
#include "utils/math.h"
#include <stddef.h>
#include <stdint.h>

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
  scaled = clamp_i32(scaled, -MAG_MEAS_MAX_DEC, MAG_MEAS_MAX_DEC);
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

static uint32_t ahrs_alpha_q15(void) {
  return float_to_q15(g_ahrs.config.ema_alpha);
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

    g_ahrs.smoothed_imu_value.accel_x =
        ema_blend_i16_q15(g_ahrs.smoothed_imu_value.accel_x, clamp_i16(cx), aq);
    g_ahrs.smoothed_imu_value.accel_y =
        ema_blend_i16_q15(g_ahrs.smoothed_imu_value.accel_y, clamp_i16(cy), aq);
    g_ahrs.smoothed_imu_value.accel_z =
        ema_blend_i16_q15(g_ahrs.smoothed_imu_value.accel_z, clamp_i16(cz), aq);
    g_ahrs.smoothed_imu_value.gyro_x = ema_blend_i16_q15(
        g_ahrs.smoothed_imu_value.gyro_x, imu_value->gyro_x, aq);
    g_ahrs.smoothed_imu_value.gyro_y = ema_blend_i16_q15(
        g_ahrs.smoothed_imu_value.gyro_y, imu_value->gyro_y, aq);
    g_ahrs.smoothed_imu_value.gyro_z = ema_blend_i16_q15(
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
    g_ahrs.smoothed_magnetometer_value.magnet_x = ema_blend_i16_q15(
        g_ahrs.smoothed_magnetometer_value.magnet_x, corr[0], aq);
    g_ahrs.smoothed_magnetometer_value.magnet_y = ema_blend_i16_q15(
        g_ahrs.smoothed_magnetometer_value.magnet_y, corr[1], aq);
    g_ahrs.smoothed_magnetometer_value.magnet_z = ema_blend_i16_q15(
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
      const double heading_rad = atan2_rad((double)my, (double)mx);
      if (!is_nan_f64(heading_rad)) {
        const double heading_deg = normalize_deg_0_360(rad_to_deg(heading_rad));
        out.heading = heading_deg;
        out.yaw = heading_deg;
      }
    }

    // calculate pitch and roll
    if (g_ahrs.imu_state == AHRS_STATE_ACTIVE) {
      if ((g_ahrs.smoothed_imu_value.accel_z != 0) ||
          (g_ahrs.smoothed_imu_value.accel_y != 0)) {
        const double pitch_rad =
            atan2_rad((double)g_ahrs.smoothed_imu_value.accel_y,
                      (double)g_ahrs.smoothed_imu_value.accel_z);
        if (!is_nan_f64(pitch_rad)) {
          out.pitch = rad_to_deg(pitch_rad);
        }
      }
      if ((g_ahrs.smoothed_imu_value.accel_x != 0) ||
          (g_ahrs.smoothed_imu_value.accel_z != 0)) {
        const double roll_rad =
            atan2_rad((double)g_ahrs.smoothed_imu_value.accel_x,
                      (double)g_ahrs.smoothed_imu_value.accel_z);
        if (!is_nan_f64(roll_rad)) {
          out.roll = rad_to_deg(roll_rad);
        }
      }
    }
  }

  return out;
}
