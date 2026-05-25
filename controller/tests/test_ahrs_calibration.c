/**
 * AHRS calibration suite — registers tests via
 * ahrs_calibration_register_tests().
 */

#include "integrations/ahrs.h"
#include "test_runner.h"
#include <string.h>

#define AHRS_CALIB_SUITE "ahrs_calibration"

static void ahrs_fixture_reset(float alpha) {
  (void)memset(&g_ahrs, 0, sizeof(g_ahrs));
  g_ahrs.config.ema_alpha = alpha;
}

TEST(ahrs_init_rejects_invalid_alpha) {
  ahrs_fixture_reset(0.5f);
  g_ahrs.config.ema_alpha = 1.0f;
  ASSERT(ahrs_init() == AHRS_RESULT_ERROR_INVALID_CONFIG);
  g_ahrs.config.ema_alpha = 0.0f;
  ASSERT(ahrs_init() == AHRS_RESULT_ERROR_INVALID_CONFIG);
  g_ahrs.config.ema_alpha = 0.5f;
  ASSERT(ahrs_init() == AHRS_RESULT_OK);
  return 0;
}

TEST(calibrate_imu_rejects_until_active) {
  ahrs_fixture_reset(0.99f);
  ASSERT(ahrs_init() == AHRS_RESULT_OK);
  ASSERT(g_ahrs.imu_state == AHRS_STATE_INITIALIZING);
  ASSERT(ahrs_calibrate_imu() == AHRS_CALIBRATION_RESULT_FAILED);

  imu_value_t imu = {.accel_x = 100,
                     .accel_y = 200,
                     .accel_z = 300,
                     .gyro_x = 0,
                     .gyro_y = 0,
                     .gyro_z = 0};
  ASSERT(ahrs_update_imu(&imu) == AHRS_RESULT_OK);
  ASSERT(g_ahrs.imu_state == AHRS_STATE_ACTIVE);
  ASSERT(ahrs_calibrate_imu() == AHRS_CALIBRATION_RESULT_IN_PROGRESS);
  ASSERT(g_ahrs.imu_state == AHRS_STATE_CALIBRATING);
  return 0;
}

TEST(imu_calibration_snapshot_matches_smoothed_accel) {
  ahrs_fixture_reset(0.99f);
  ASSERT(ahrs_init() == AHRS_RESULT_OK);

  imu_value_t imu = {.accel_x = 1000,
                     .accel_y = 2000,
                     .accel_z = -500,
                     .gyro_x = 10,
                     .gyro_y = -20,
                     .gyro_z = 30};
  ASSERT(ahrs_update_imu(&imu) == AHRS_RESULT_OK);
  ASSERT(g_ahrs.imu_state == AHRS_STATE_ACTIVE);

  ASSERT(ahrs_calibrate_imu() == AHRS_CALIBRATION_RESULT_IN_PROGRESS);
  ASSERT(g_ahrs.imu_state == AHRS_STATE_CALIBRATING);

  ASSERT(ahrs_update_imu(&imu) == AHRS_RESULT_OK);
  ASSERT(g_ahrs.imu_state == AHRS_STATE_ACTIVE);

  ASSERT_EQ_I32(g_ahrs.imu_calibration.bias[0],
                g_ahrs.smoothed_imu_value.accel_x);
  ASSERT_EQ_I32(g_ahrs.imu_calibration.bias[1],
                g_ahrs.smoothed_imu_value.accel_y);
  ASSERT_EQ_I32(g_ahrs.imu_calibration.bias[2],
                g_ahrs.smoothed_imu_value.accel_z);

  ASSERT(ahrs_update_imu(&imu) == AHRS_RESULT_OK);
  ASSERT(g_ahrs.smoothed_imu_value.accel_x > -50 &&
         g_ahrs.smoothed_imu_value.accel_x < 50);
  ASSERT(g_ahrs.smoothed_imu_value.accel_y > -50 &&
         g_ahrs.smoothed_imu_value.accel_y < 50);
  ASSERT(g_ahrs.smoothed_imu_value.accel_z > -50 &&
         g_ahrs.smoothed_imu_value.accel_z < 50);
  return 0;
}

TEST(calibrate_compass_rejects_until_mag_active) {
  ahrs_fixture_reset(0.5f);
  ASSERT(ahrs_init() == AHRS_RESULT_OK);
  ASSERT(g_ahrs.magnetometer_state == AHRS_STATE_INITIALIZING);
  ASSERT(ahrs_calibrate_compass(NULL) == AHRS_CALIBRATION_RESULT_FAILED);

  magnetometer_value_t mag = {10, 20, 30};
  ASSERT(ahrs_update_magnetometer(&mag) == AHRS_RESULT_OK);
  ASSERT(g_ahrs.magnetometer_state == AHRS_STATE_ACTIVE);
  ASSERT(ahrs_calibrate_compass(NULL) == AHRS_CALIBRATION_RESULT_IN_PROGRESS);
  ASSERT(g_ahrs.magnetometer_state == AHRS_STATE_CALIBRATING);
  return 0;
}

TEST(magnetometer_calibration_completes_with_axis_excursion) {
  ahrs_fixture_reset(0.5f);
  ASSERT(ahrs_init() == AHRS_RESULT_OK);

  magnetometer_value_t mag0 = {0, 0, 0};
  ASSERT(ahrs_update_magnetometer(&mag0) == AHRS_RESULT_OK);
  ASSERT(g_ahrs.magnetometer_state == AHRS_STATE_ACTIVE);

  ASSERT(ahrs_calibrate_compass(NULL) == AHRS_CALIBRATION_RESULT_IN_PROGRESS);

  magnetometer_value_t low = {-400, -400, -400};
  magnetometer_value_t high = {400, 400, 400};

  for (int i = 0; i < 160; i++) {
    magnetometer_value_t *m = (i % 2 == 0) ? &low : &high;
    ahrs_result_t r = ahrs_update_magnetometer(m);
    ASSERT(r == AHRS_RESULT_OK);
    if (g_ahrs.magnetometer_state == AHRS_STATE_ACTIVE) {
      ASSERT(i >= AHRS_MAG_CAL_MIN_SAMPLES - 1);
      break;
    }
    ASSERT(g_ahrs.magnetometer_state == AHRS_STATE_CALIBRATING);
  }

  ASSERT(g_ahrs.magnetometer_state == AHRS_STATE_ACTIVE);

  ASSERT(g_ahrs.magnetometer_calibration.bias[0] > -5 &&
         g_ahrs.magnetometer_calibration.bias[0] < 5);
  ASSERT(g_ahrs.magnetometer_calibration.bias[1] > -5 &&
         g_ahrs.magnetometer_calibration.bias[1] < 5);
  ASSERT(g_ahrs.magnetometer_calibration.bias[2] > -5 &&
         g_ahrs.magnetometer_calibration.bias[2] < 5);

  ASSERT(g_ahrs.magnetometer_calibration.scale_den[0] > 0);
  ASSERT(g_ahrs.magnetometer_calibration.scale_den[1] > 0);
  ASSERT(g_ahrs.magnetometer_calibration.scale_den[2] > 0);
  ASSERT(g_ahrs.magnetometer_calibration.scale_num[0] > 0);
  return 0;
}

TEST(magnetometer_calibration_no_finish_if_span_too_small) {
  ahrs_fixture_reset(0.5f);
  ASSERT(ahrs_init() == AHRS_RESULT_OK);
  magnetometer_value_t mag0 = {50, 50, 50};
  ASSERT(ahrs_update_magnetometer(&mag0) == AHRS_RESULT_OK);
  ASSERT(ahrs_calibrate_compass(NULL) == AHRS_CALIBRATION_RESULT_IN_PROGRESS);

  magnetometer_value_t same = {100, 100, 100};
  for (int i = 0; i < 200; i++) {
    ASSERT(ahrs_update_magnetometer(&same) == AHRS_RESULT_OK);
  }
  ASSERT(g_ahrs.magnetometer_state == AHRS_STATE_CALIBRATING);
  ASSERT(g_ahrs.mag_cal_sample_count == 200);
  return 0;
}

TEST(magnetometer_identity_calibration_preserves_centered_reading) {
  ahrs_fixture_reset(0.5f);
  ASSERT(ahrs_init() == AHRS_RESULT_OK);
  magnetometer_value_t m = {MAG_MEAS_MAX_DEC / 2, MAG_MEAS_MAX_DEC / 2, 0};
  ASSERT(ahrs_update_magnetometer(&m) == AHRS_RESULT_OK);
  ASSERT(g_ahrs.smoothed_magnetometer_value.magnet_x == m.magnet_x);
  ASSERT(g_ahrs.smoothed_magnetometer_value.magnet_y == m.magnet_y);
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
  test_registry_add(
      reg, AHRS_CALIB_SUITE,
      "magnetometer_calibration_completes_with_axis_excursion",
      test_magnetometer_calibration_completes_with_axis_excursion);
  test_registry_add(reg, AHRS_CALIB_SUITE,
                    "magnetometer_calibration_no_finish_if_span_too_small",
                    test_magnetometer_calibration_no_finish_if_span_too_small);
  test_registry_add(
      reg, AHRS_CALIB_SUITE,
      "magnetometer_identity_calibration_preserves_centered_reading",
      test_magnetometer_identity_calibration_preserves_centered_reading);
}
