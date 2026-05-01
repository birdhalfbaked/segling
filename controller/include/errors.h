#ifndef ERRORS_H
#define ERRORS_H

/// @defgroup ERRORS_types ERRORS types
/// @{

/// Error code
/// @brief Error code
typedef enum {
  AHRS_RESULT_OK = 0,
  AHRS_RESULT_ERROR_UNKNOWN = -1,
  AHRS_RESULT_ERROR_INVALID_CONFIG = -2,
  AHRS_RESULT_ERROR_CALIBRATION_FAILED = -3,
  AHRS_RESULT_ERROR_UPDATE_FAILED = -4,
} ahrs_result_t;

typedef enum {
  STORAGE_RESULT_OK = 0,
  STORAGE_RESULT_ERROR_UNKNOWN = -1,
  STORAGE_RESULT_ERROR_LOAD_FAILED = -2,
} storage_result_t;

typedef enum {
  COMMS_RESULT_OK = 0,
  COMMS_RESULT_ERROR_UNKNOWN = -1,
  COMMS_RESULT_ERROR_READ_FAILED = -2,
  COMMS_RESULT_ERROR_WRITE_FAILED = -3,
} comms_result_t;

/// @}
#endif