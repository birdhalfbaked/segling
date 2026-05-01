#ifndef AHRS_H
#define AHRS_H

#include "errors.h"
#include <stdint.h>

#define ACCELEROMETER_ADDRESS 0x68
#define ACCELEROMETER_START_REGISTER 0x3B
#define ACCELEROMETER_DATA_SIZE 14

#define MAGNETOMETER_ADDRESS 0x0C
#define MAGNETOMETER_START_REGISTER 0x02
#define MAGNETOMETER_DATA_SIZE 6

/// @defgroup AHRS_types AHRS types
/// @{

/// IMU data structure
/// @brief IMU data structure
/// @param rotation Quaternion representing the rotation of the sensor
/// @param rotation_rate Rotation rate in rad/s about the x, y, and z axes
typedef struct {
  float rotation[4];
  float rotation_rate[3];
} imu_t;

/// IMU value structure
/// @brief IMU value structure
/// @param accel_x Accelerometer x value
/// @param accel_y Accelerometer y value
/// @param accel_z Accelerometer z value
/// @param gyro_x Gyroscope x value
/// @param gyro_y Gyroscope y value
/// @param gyro_z Gyroscope z value
typedef struct {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
} imu_value_t;

/// IMU calibration structure
/// @brief IMU calibration structure
/// @param bias Bias in the scaled sensor units
typedef struct {
  float bias[3];
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

/// Magnetometer calibration structure
/// @brief Magnetometer calibration structure
/// @param bias[3] Bias in the scaled sensor units
/// @param mapping[3][3] Mapping matrix
typedef struct {
  float bias[3];
  float mapping[3][3];
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
  float heading;
  float pitch;
  float roll;
  float yaw;
  float rotation_rate_x;
  float rotation_rate_y;
  float rotation_rate_z;
} ahrs_public_t;

/// AHRS configuration structure
/// @brief AHRS configuration structure
/// @param ema_alpha EMA alpha
typedef struct {
  float ema_alpha;

} ahrs_config_t;

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
  ahrs_config_t *config;

  ahrs_state_t imu_state;
  imu_calibration_t imu_calibration;
  imu_value_t smoothed_imu_value;

  ahrs_state_t magnetometer_state;
  magnetometer_calibration_t magnetometer_calibration;
  magnetometer_value_t smoothed_magnetometer_value;

  float temperature;
} ahrs_t;

/// @defgroup AHRS_methods AHRS methods
/// @{

ahrs_result_t ahrs_init(ahrs_t *ahrs);
ahrs_result_t ahrs_calibrate_compass(ahrs_t *ahrs);
ahrs_result_t ahrs_calibrate_imu(ahrs_t *ahrs);

ahrs_result_t ahrs_update_imu(ahrs_t *ahrs, imu_value_t *imu_value);

ahrs_result_t
ahrs_update_magnetometer(ahrs_t *ahrs,
                         magnetometer_value_t *magnetometer_value);

ahrs_public_t ahrs_get_data(ahrs_t *ahrs);

/// @}
#endif