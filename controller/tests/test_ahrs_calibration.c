/**
 * AHRS calibration suite — registers tests via ahrs_calibration_register_tests().
 */

#include "ahrs.h"
#include "test_runner.h"
#include <string.h>

#define AHRS_CALIB_SUITE "ahrs_calibration"

static void ahrs_fixture_zero(ahrs_t *ahrs, ahrs_config_t *cfg) {
  memset(ahrs, 0, sizeof(*ahrs));
  memset(cfg, 0, sizeof(*cfg));
  cfg->ema_alpha = 0.5f;
  ahrs->config = cfg;
  /* magnetometer_calibration all zero => scale_den 0 => identity scaling */
}

TEST(ahrs_init_rejects_invalid_alpha) {
  ahrs_t ahrs;
  ahrs_config_t cfg;
  ahrs_fixture_zero(&ahrs, &cfg);
  cfg.ema_alpha = 1.0f;
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_ERROR_INVALID_CONFIG);
  cfg.ema_alpha = 0.0f;
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_ERROR_INVALID_CONFIG);
  cfg.ema_alpha = 0.5f;
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_OK);
  return 0;
}

TEST(calibrate_imu_rejects_until_active) {
  ahrs_t ahrs;
  ahrs_config_t cfg;
  ahrs_fixture_zero(&ahrs, &cfg);
  cfg.ema_alpha = 0.99f;
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_OK);
  ASSERT(ahrs.imu_state == AHRS_STATE_INITIALIZING);
  ASSERT(ahrs_calibrate_imu(&ahrs) == AHRS_RESULT_ERROR_CALIBRATION_FAILED);

  imu_value_t imu = {.accel_x = 100,
                     .accel_y = 200,
                     .accel_z = 300,
                     .gyro_x = 0,
                     .gyro_y = 0,
                     .gyro_z = 0};
  ASSERT(ahrs_update_imu(&ahrs, &imu) == AHRS_RESULT_OK);
  ASSERT(ahrs.imu_state == AHRS_STATE_ACTIVE);
  ASSERT(ahrs_calibrate_imu(&ahrs) == AHRS_RESULT_OK);
  ASSERT(ahrs.imu_state == AHRS_STATE_CALIBRATING);
  return 0;
}

TEST(imu_calibration_snapshot_matches_smoothed_accel) {
  ahrs_t ahrs;
  ahrs_config_t cfg;
  ahrs_fixture_zero(&ahrs, &cfg);
  cfg.ema_alpha = 0.99f;
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_OK);

  imu_value_t imu = {.accel_x = 1000,
                     .accel_y = 2000,
                     .accel_z = -500,
                     .gyro_x = 10,
                     .gyro_y = -20,
                     .gyro_z = 30};
  ASSERT(ahrs_update_imu(&ahrs, &imu) == AHRS_RESULT_OK);
  ASSERT(ahrs.imu_state == AHRS_STATE_ACTIVE);

  ASSERT(ahrs_calibrate_imu(&ahrs) == AHRS_RESULT_OK);
  ASSERT(ahrs.imu_state == AHRS_STATE_CALIBRATING);

  ASSERT(ahrs_update_imu(&ahrs, &imu) == AHRS_RESULT_OK);
  ASSERT(ahrs.imu_state == AHRS_STATE_ACTIVE);

  ASSERT_EQ_I32(ahrs.imu_calibration.bias[0], ahrs.smoothed_imu_value.accel_x);
  ASSERT_EQ_I32(ahrs.imu_calibration.bias[1], ahrs.smoothed_imu_value.accel_y);
  ASSERT_EQ_I32(ahrs.imu_calibration.bias[2], ahrs.smoothed_imu_value.accel_z);

  /* Same physical accel after bias removal should drive smoothed toward ~0 */
  ASSERT(ahrs_update_imu(&ahrs, &imu) == AHRS_RESULT_OK);
  ASSERT(ahrs.smoothed_imu_value.accel_x > -50 &&
         ahrs.smoothed_imu_value.accel_x < 50);
  ASSERT(ahrs.smoothed_imu_value.accel_y > -50 &&
         ahrs.smoothed_imu_value.accel_y < 50);
  ASSERT(ahrs.smoothed_imu_value.accel_z > -50 &&
         ahrs.smoothed_imu_value.accel_z < 50);
  return 0;
}

TEST(calibrate_compass_rejects_until_mag_active) {
  ahrs_t ahrs;
  ahrs_config_t cfg;
  ahrs_fixture_zero(&ahrs, &cfg);
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_OK);
  ASSERT(ahrs.magnetometer_state == AHRS_STATE_INITIALIZING);
  ASSERT(ahrs_calibrate_compass(&ahrs) == AHRS_RESULT_ERROR_CALIBRATION_FAILED);

  magnetometer_value_t mag = {10, 20, 30};
  ASSERT(ahrs_update_magnetometer(&ahrs, &mag) == AHRS_RESULT_OK);
  ASSERT(ahrs.magnetometer_state == AHRS_STATE_ACTIVE);
  ASSERT(ahrs_calibrate_compass(&ahrs) == AHRS_RESULT_OK);
  ASSERT(ahrs.magnetometer_state == AHRS_STATE_CALIBRATING);
  return 0;
}

TEST(magnetometer_calibration_completes_with_axis_excursion) {
  ahrs_t ahrs;
  ahrs_config_t cfg;
  ahrs_fixture_zero(&ahrs, &cfg);
  cfg.ema_alpha = 0.5f;
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_OK);

  magnetometer_value_t mag0 = {0, 0, 0};
  ASSERT(ahrs_update_magnetometer(&ahrs, &mag0) == AHRS_RESULT_OK);
  ASSERT(ahrs.magnetometer_state == AHRS_STATE_ACTIVE);

  ASSERT(ahrs_calibrate_compass(&ahrs) == AHRS_RESULT_OK);

  magnetometer_value_t low = {-400, -400, -400};
  magnetometer_value_t high = {400, 400, 400};

  for (int i = 0; i < 160; i++) {
    magnetometer_value_t *m = (i % 2 == 0) ? &low : &high;
    ahrs_result_t r = ahrs_update_magnetometer(&ahrs, m);
    ASSERT(r == AHRS_RESULT_OK);
    if (ahrs.magnetometer_state == AHRS_STATE_ACTIVE) {
      ASSERT(i >= AHRS_MAG_CAL_MIN_SAMPLES - 1);
      break;
    }
    ASSERT(ahrs.magnetometer_state == AHRS_STATE_CALIBRATING);
  }

  ASSERT(ahrs.magnetometer_state == AHRS_STATE_ACTIVE);

  /* Symmetric swing around zero → bias centers near 0 */
  ASSERT(ahrs.magnetometer_calibration.bias[0] > -5 &&
         ahrs.magnetometer_calibration.bias[0] < 5);
  ASSERT(ahrs.magnetometer_calibration.bias[1] > -5 &&
         ahrs.magnetometer_calibration.bias[1] < 5);
  ASSERT(ahrs.magnetometer_calibration.bias[2] > -5 &&
         ahrs.magnetometer_calibration.bias[2] < 5);

  ASSERT(ahrs.magnetometer_calibration.scale_den[0] > 0);
  ASSERT(ahrs.magnetometer_calibration.scale_den[1] > 0);
  ASSERT(ahrs.magnetometer_calibration.scale_den[2] > 0);
  ASSERT(ahrs.magnetometer_calibration.scale_num[0] > 0);
  return 0;
}

TEST(magnetometer_calibration_no_finish_if_span_too_small) {
  ahrs_t ahrs;
  ahrs_config_t cfg;
  ahrs_fixture_zero(&ahrs, &cfg);
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_OK);
  magnetometer_value_t mag0 = {50, 50, 50};
  ASSERT(ahrs_update_magnetometer(&ahrs, &mag0) == AHRS_RESULT_OK);
  ASSERT(ahrs_calibrate_compass(&ahrs) == AHRS_RESULT_OK);

  magnetometer_value_t same = {100, 100, 100};
  for (int i = 0; i < 200; i++) {
    ASSERT(ahrs_update_magnetometer(&ahrs, &same) == AHRS_RESULT_OK);
  }
  ASSERT(ahrs.magnetometer_state == AHRS_STATE_CALIBRATING);
  ASSERT(ahrs.mag_cal_sample_count == 200);
  return 0;
}

TEST(magnetometer_identity_calibration_preserves_centered_reading) {
  ahrs_t ahrs;
  ahrs_config_t cfg;
  ahrs_fixture_zero(&ahrs, &cfg);
  ASSERT(ahrs_init(&ahrs) == AHRS_RESULT_OK);
  magnetometer_value_t m = {MAG_MEAS_MAX_DEC / 2, MAG_MEAS_MIN_DEC / 2, 0};
  ASSERT(ahrs_update_magnetometer(&ahrs, &m) == AHRS_RESULT_OK);
  ASSERT(ahrs.smoothed_magnetometer_value.magnet_x == m.magnet_x);
  ASSERT(ahrs.smoothed_magnetometer_value.magnet_y == m.magnet_y);
  return 0;
}

void ahrs_calibration_register_tests(test_registry_t *reg) {
  test_registry_add(reg, AHRS_CALIB_SUITE, "ahrs_init_rejects_invalid_alpha",
                    test_ahrs_init_rejects_invalid_alpha);
  test_registry_add(reg, AHRS_CALIB_SUITE, "calibrate_imu_rejects_until_active",
                    test_calibrate_imu_rejects_until_active);
  test_registry_add(reg, AHRS_CALIB_SUITE,
                    "imu_calibration_snapshot_matches_smoothed_accel",
                    test_imu_calibration_snapshot_matches_smoothed_accel);
  test_registry_add(reg, AHRS_CALIB_SUITE,
                    "calibrate_compass_rejects_until_mag_active",
                    test_calibrate_compass_rejects_until_mag_active);
  test_registry_add(reg, AHRS_CALIB_SUITE,
                    "magnetometer_calibration_completes_with_axis_excursion",
                    test_magnetometer_calibration_completes_with_axis_excursion);
  test_registry_add(reg, AHRS_CALIB_SUITE,
                    "magnetometer_calibration_no_finish_if_span_too_small",
                    test_magnetometer_calibration_no_finish_if_span_too_small);
  test_registry_add(reg, AHRS_CALIB_SUITE,
                    "magnetometer_identity_calibration_preserves_centered_reading",
                    test_magnetometer_identity_calibration_preserves_centered_reading);
}
