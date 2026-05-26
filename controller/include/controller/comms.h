/// @defgroup COMMS Communications module
/// @ingroup CORE
/// @brief Communication with external devices.
/// Build with @c COMMS_BACKEND=HARDWARE or @c SIM in the Makefile (maps to the
/// numeric @c COMMS_BACKEND token below). Both @c comms_hardware.c and
/// @c comms_sim.c are compiled; the inactive file is empty after preprocessing.
/// @{
#ifndef COMMS_H
#define COMMS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Makefile: COMMS_BACKEND=HARDWARE|SIM → -DCOMMS_BACKEND=COMMS_BACKEND_* */
#define COMMS_BACKEND_HARDWARE (1)
#define COMMS_BACKEND_SIM (2)

#ifndef COMMS_BACKEND
#define COMMS_BACKEND COMMS_BACKEND_SIM
#endif

#ifndef I2C_DEVICE
#define I2C_DEVICE "/dev/i2c-1"
#endif

#ifndef COMMS_BUFFER_SIZE
#define COMMS_BUFFER_SIZE (128U)
#endif

typedef enum {
  COMMS_RESULT_OK = 0,
  COMMS_RESULT_ERROR_UNKNOWN = -1,
  COMMS_RESULT_ERROR_READ_FAILED = -2,
  COMMS_RESULT_ERROR_WRITE_FAILED = -3,
} comms_result_t;

typedef struct {
  bool _initialized;
  int _i2c_fd;
} comms_t;

comms_result_t comms_init(comms_t *comms);
comms_result_t comms_deinit(comms_t *comms);
comms_result_t comms_read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                              uint8_t *data, size_t size);
comms_result_t comms_write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                               const uint8_t *data, size_t size);

#endif
/// @}
