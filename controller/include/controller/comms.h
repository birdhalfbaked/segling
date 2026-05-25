/// @defgroup COMMS Communications module
/// @ingroup CORE
/// @brief This module is used to facilitate communication with the external
/// devices over various buses.
/// @{
#ifndef COMMS_H
#define COMMS_H

// Includes

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Types

/// @brief I2C device default bus
/// @details The default I2C bus is /dev/i2c-1 as per raspberry pi
/// documentation. This can be overridden at compile time via -DI2C_DEVICE=... .
#ifndef I2C_DEVICE
#define I2C_DEVICE "/dev/i2c-1"
#endif

/// @brief COMMS result codes that can be returned by the comms functions
typedef enum {
  /// @brief Success
  COMMS_RESULT_OK = 0,
  /// @brief Unknown or unexpected error occurred
  COMMS_RESULT_ERROR_UNKNOWN = -1,
  /// @brief Attempt to read data from the device failed
  COMMS_RESULT_ERROR_READ_FAILED = -2,
  /// @brief Attempt to write data to the device failed
  COMMS_RESULT_ERROR_WRITE_FAILED = -3,
} comms_result_t;

/// @brief Comms data structure. This is used to communicate with the external
/// devices.
typedef struct {
  /// @private
  /// @brief Flag indicating if the comms structure has been initialized
  /// @details This is used to prevent double initialization and
  /// deinitialization.
  bool _initialized;
  /// @private
  /// @brief I2C file descriptor
  /// @details This is used to communicate with the I2C devices and is
  /// internally managed.
  int _i2c_fd;
  /// @private
  /// @brief Serial file descriptor
  /// @details This is used to communicate with the Serial devices and is
  /// internally managed.
  int _serial_fd;
} comms_t;

// Functions

/// @brief Initialize the comms structure and prepare it for use
/// @memberof comms_t
/// @param comms Pointer to the comms structure
/// @pre comms must be a valid pointer to a comms_t structure
/// @return @ref COMMS_RESULT_OK if the comms was initialized \n
///         @ref COMMS_RESULT_ERROR_UNKNOWN if an unknown or unexpected error
///         occurred
comms_result_t comms_init(comms_t *comms);

/// @brief Deinitialize the comms structure and free resources
/// @memberof comms_t
/// @param comms Pointer to the comms structure
/// @pre comms must be a valid pointer to a comms_t structure
/// @pre comms must have been initialized
/// @return @ref COMMS_RESULT_OK if the comms was deinitialized \n
///         @ref COMMS_RESULT_ERROR_UNKNOWN if an unknown or unexpected error
///         occurred
comms_result_t comms_deinit(comms_t *comms);

/// @brief Read from an I2C device
/// @memberof comms_t
/// @param comms Pointer to the comms structure
/// @param address I2C address
/// @param reg Register address
/// @param data Data buffer
/// @param size Size of the data buffer
/// @pre comms must be a valid pointer to a comms_t structure
/// @pre comms must have been initialized
/// @return @ref COMMS_RESULT_OK if the data was read \n
///         @ref COMMS_RESULT_ERROR_READ_FAILED if the data
///         could not be read \n
///         @ref COMMS_RESULT_ERROR_UNKNOWN if an unknown error
///         occurred
comms_result_t comms_read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                              uint8_t *data, size_t size);
/// @brief Write to an I2C device
/// @memberof comms_t
/// @param comms Pointer to the comms structure
/// @param address I2C address
/// @param reg Register address
/// @param data Data buffer
/// @param size Size of the data buffer
/// @pre comms must be a valid pointer to a comms_t structure
/// @pre comms must have been initialized
/// @return @ref COMMS_RESULT_OK if the data was written \n
///         @ref COMMS_RESULT_ERROR_WRITE_FAILED if the data
///         could not be written \n
///         @ref COMMS_RESULT_ERROR_UNKNOWN if an unknown or unexpected error
///         occurred
comms_result_t comms_write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                               uint8_t *data, size_t size);

#endif
/// @}
