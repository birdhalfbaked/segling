#ifndef COMMS_H
#define COMMS_H

#include <stddef.h>
#include <stdint.h>

/// @defgroup COMMS_types COMMS types
/// @{

typedef enum {
  COMMS_RESULT_OK = 0,
  COMMS_RESULT_ERROR_UNKNOWN = -1,
  COMMS_RESULT_ERROR_READ_FAILED = -2,
  COMMS_RESULT_ERROR_WRITE_FAILED = -3,
} comms_result_t;

/// I2C device
/// @brief I2C device default bus
#define I2C_DEVICE "/dev/i2c-1"

/// Comms structure
/// @brief Comms structure
/// @param i2c_fd I2C file descriptor
/// @param serial_fd Serial file descriptor
typedef struct {
  int i2c_fd;
  int serial_fd;
} comms_t;
/// @}

/// @defgroup COMMS_methods COMMS methods
/// @{
/// Initialize the comms
/// @brief Initialize the comms
/// @param comms Comms
/// @return COMMS_RESULT_OK if the comms was initialized,
/// COMMS_RESULT_ERROR_UNKNOWN if an error occurred
comms_result_t comms_init(comms_t *comms);

/// Deinitialize the comms
/// @brief Deinitialize the comms
/// @param comms Comms
/// @return COMMS_RESULT_OK if the comms was deinitialized,
/// COMMS_RESULT_ERROR_UNKNOWN if an error occurred
comms_result_t comms_deinit(comms_t *comms);

/// Read from an I2C device
/// @brief Read from an I2C device
/// @param comms Comms
/// @param address I2C address
/// @param reg Register address
/// @param data Data buffer
/// @param size Size of the data buffer
/// @return COMMS_RESULT_OK if the data was read,
/// COMMS_RESULT_ERROR_UNKNOWN if an error occurred
comms_result_t read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                        uint8_t *data, size_t size);
/// Write to an I2C device
/// @brief Write to an I2C device
/// @param comms Comms
/// @param address I2C address
/// @param reg Register address
/// @param data Data buffer
/// @param size Size of the data buffer
/// @return COMMS_RESULT_OK if the data was written,
/// COMMS_RESULT_ERROR_UNKNOWN if an error occurred
comms_result_t write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                         uint8_t *data, size_t size);

#endif