#ifndef STORAGE_H
#define STORAGE_H

#include "errors.h"
#include <stdint.h>

/// @defgroup STORAGE_types STORAGE types
/// @{

#define FILE_ID_IMU 0
#define FILE_ID_MAGNETOMETER 1
#define FILE_ID_GPS 2
#define FILE_ID_BAROMETER 3
#define FILE_ID_IMU_CALIBRATION 8
#define FILE_ID_MAGNETOMETER_CALIBRATION 9

/// Memory map storage scheme with "files" of 128 bytes
#define STORAGE_MAX_FILES 16
#define STORAGE_FILE_SIZE 128

/// File identifier
/// @brief File identifier, used to identify the file in the memory map
typedef uint8_t file_id_t;

/// Memory map storage structure
/// @brief Memory map storage structure
/// @param files Array of files, each file is 128 bytes
typedef struct {
  uint8_t *files;
} storage_t;

/// @}

/// @defgroup STORAGE_methods STORAGE methods
/// @{
storage_result_t storage_init(storage_t *storage);
storage_result_t storage_write(storage_t *storage, file_id_t file_id,
                               uint8_t *data);
storage_result_t storage_read(storage_t *storage, file_id_t file_id,
                              uint8_t *data);
/// @}

#endif