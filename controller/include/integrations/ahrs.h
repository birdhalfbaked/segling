#ifndef INTEGRATIONS_AHRS_H
#define INTEGRATIONS_AHRS_H

#include "controller/controller.h"
#include <stdint.h>

#define ACCELEROMETER_ADDRESS 0x68
#define ACCELEROMETER_START_REGISTER 0x3B
#define ACCELEROMETER_DATA_SIZE 14

#define MAGNETOMETER_ADDRESS 0x0C
#define MAGNETOMETER_START_REGISTER 0x02
#define MAGNETOMETER_DATA_SIZE 6

/// Magnetometer samples are two's complement, little-endian on the bus;
/// each axis is in [MAG_MEAS_MIN_DEC, MAG_MEAS_MAX_DEC] (decimal).
#define MAG_MEAS_MIN_DEC (-4096)
#define MAG_MEAS_MAX_DEC (4095)

/// Compass calibration: collect raw extrema while turning; then bias +
/// per-axis rational soft-iron (diagonal only).
#define AHRS_MAG_CAL_MIN_SAMPLES 150
#define AHRS_MAG_CAL_MIN_SPAN 64
#define AHRS_MAG_CAL_MAX_SAMPLES 16000
#define AHRS_AXIS_COUNT (3U)

/// @defgroup AHRS_types AHRS types
/// @{

typedef enum {
  AHRS_RESULT_OK = 0,
  AHRS_RESULT_ERROR_UNKNOWN = -1,
  AHRS_RESULT_ERROR_INVALID_CONFIG = -2,
  AHRS_RESULT_ERROR_CALIBRATION_FAILED = -3,
  AHRS_RESULT_ERROR_UPDATE_FAILED = -4,
} ahrs_result_t;

typedef enum {
  AHRS_CALIBRATION_RESULT_IN_PROGRESS = 0,
  AHRS_CALIBRATION_RESULT_COMPLETED = 1,
  AHRS_CALIBRATION_RESULT_FAILED = 2,
} ahrs_calibration_status_t;

/// AHRS state structure
/// @brief AHRS state structure
/// @param state State of the AHRS
typedef enum {
  AHRS_STATE_INITIALIZING = 0,
  AHRS_STATE_CALIBRATING = 1,
  AHRS_STATE_ACTIVE = 2,
  AHRS_STATE_DISABLED = 3,
  AHRS_STATE_FAILED = 4,
} ahrs_state_t;

/// IMU data structure
/// @brief IMU data structure
/// @param rotation Quaternion representing the rotation of the sensor
/// @param rotation_rate Rotation rate in rad/s about the x, y, and z axes
typedef struct {
  float rotation[4];
  float rotation_rate[AHRS_AXIS_COUNT];
} imu_t;

/// IMU value structure
/// @brief Raw INT16 register counts (same units as ACCEL_xOUT / GYRO_xOUT).
/// Scale to SI using ACCEL_FS / gyro full-scale settings from the IMU; this
/// layer keeps samples in LSB.
typedef struct {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
} imu_value_t;

/// IMU calibration structure
/// @brief Accelerometer bias in the same LSB units as imu_value_t accel_*.
/// Corrected accel uses (raw - bias) with int32 intermediates; single-sample
/// calibration copies smoothed accel while still.
typedef struct {
  int32_t bias[AHRS_AXIS_COUNT];
} imu_calibration_t;

/// Magnetometer data structure
/// @brief Magnetometer data structure
/// @param heading Heading in degrees
typedef struct {
  float heading;
} magnetometer_t;

/// Magnetometer value structure
/// @brief Magnetometer value structure
/// @param magnet_x Magnetometer x value
/// @param magnet_y Magnetometer y value
/// @param magnet_z Magnetometer z value
typedef struct {
  int16_t magnet_x;
  int16_t magnet_y;
  int16_t magnet_z;
} magnetometer_value_t;

/// Magnetometer calibration structure (integer sensor units).
/// @brief Per-axis: corrected = clamp(((raw - bias) * scale_num) / scale_den).
/// Identity / uncalibrated: scale_den[i] == 0 (treated as scale 1).
typedef struct {
  int16_t bias[AHRS_AXIS_COUNT];
  int16_t scale_num[AHRS_AXIS_COUNT];
  int16_t scale_den[AHRS_AXIS_COUNT];
} magnetometer_calibration_t;

/// AHRS public data structure
/// @brief AHRS public data structure
/// @param heading Heading in degrees
/// @param pitch Pitch in degrees
/// @param roll Roll in degrees
/// @param yaw Yaw in degrees
/// @param rotation_rate_x Rotation rate in rad/s about the x axis
/// @param rotation_rate_y Rotation rate in rad/s about the y axis
/// @param rotation_rate_z Rotation rate in rad/s about the z axis
typedef struct {
  ahrs_state_t imu_state;
  ahrs_state_t magnetometer_state;
  double heading;
  double pitch;
  double roll;
  double yaw;
  double rotation_rate_x;
  double rotation_rate_y;
  double rotation_rate_z;
} ahrs_public_t;

typedef struct {
  float ema_alpha;
} ahrs_config_t;

/// AHRS data structure
/// @brief AHRS data structure
/// @param imu IMU data
/// @param imu_calibration IMU calibration data
/// @param smoothed_imu_value Smoothed IMU value (via EMA)
/// @param magnetometer Magnetometer data
/// @param magnetometer_calibration Magnetometer calibration data
/// @param smoothed_magnetometer_value Smoothed Magnetometer value (via EMA)
/// @param temperature Temperature in Celsius
typedef struct {
  ahrs_config_t config;

  ahrs_state_t imu_state;
  imu_calibration_t imu_calibration;
  imu_value_t smoothed_imu_value;

  ahrs_state_t magnetometer_state;
  magnetometer_calibration_t magnetometer_calibration;
  magnetometer_value_t smoothed_magnetometer_value;

  float temperature;

  /// Internal: magnetometer min/max while AHRS_STATE_CALIBRATING (raw samples).
  int16_t mag_cal_min[AHRS_AXIS_COUNT];
  int16_t mag_cal_max[AHRS_AXIS_COUNT];
  uint16_t mag_cal_sample_count;
  uint8_t mag_cal_have_bounds;
} ahrs_t;

extern ahrs_t g_ahrs;

/// @defgroup AHRS_methods AHRS methods
/// @{

ahrs_result_t ahrs_init(void);

/// Begin compass calibration (magnetometer_value may be NULL), or collect one
/// sample while AHRS_STATE_CALIBRATING.
ahrs_calibration_status_t
ahrs_calibrate_compass(const magnetometer_value_t *magnetometer_value);
void ahrs_reset_compass_calibration(void);

/// Begin IMU calibration, or finish bias capture while AHRS_STATE_CALIBRATING
/// (call after the sample has been smoothed in ahrs_update_imu).
ahrs_calibration_status_t ahrs_calibrate_imu(void);
void ahrs_reset_imu_calibration(void);

ahrs_result_t ahrs_update_imu(const imu_value_t *imu_value);

ahrs_result_t
ahrs_update_magnetometer(const magnetometer_value_t *magnetometer_value);

ahrs_public_t ahrs_get_data(void);

/// Step the AHRS
/// @brief Step the AHRS
/// @param controller Controller
/// @param epoch Epoch
void ahrs_step(controller_t *controller, epoch_t epoch);

/// @}
#endif /* INTEGRATIONS_AHRS_H */
