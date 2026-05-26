/// @file comms_hardware.c
/// @brief Linux I2C backend when COMMS_BACKEND == COMMS_BACKEND_HARDWARE.

#include "controller/comms.h"

#if COMMS_BACKEND == COMMS_BACKEND_HARDWARE

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

comms_result_t comms_init(comms_t *comms) {
  comms_result_t result = COMMS_RESULT_OK;

  if (comms == NULL) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else {
    (void)memset(comms, 0, sizeof(*comms));
    comms->_i2c_fd = open(I2C_DEVICE, O_RDWR);
    if (comms->_i2c_fd < 0) {
      comms->_i2c_fd = -1;
      result = COMMS_RESULT_ERROR_READ_FAILED;
    } else {
      comms->_initialized = true;
    }
  }
  return result;
}

comms_result_t comms_deinit(comms_t *comms) {
  if ((comms != NULL) && comms->_initialized && (comms->_i2c_fd >= 0)) {
    (void)close(comms->_i2c_fd);
    comms->_i2c_fd = -1;
    comms->_initialized = false;
  }
  return COMMS_RESULT_OK;
}

comms_result_t comms_read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                              uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  struct i2c_msg msgs[2];
  struct i2c_rdwr_ioctl_data msgset;

  if ((comms == NULL) || (data == NULL) || (size == 0U) ||
      (size > COMMS_BUFFER_SIZE) || !comms->_initialized ||
      (comms->_i2c_fd < 0)) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else {
    msgs[0].addr = address;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = address;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = (uint16_t)size;
    msgs[1].buf = data;

    msgset.msgs = msgs;
    msgset.nmsgs = 2;

    if (ioctl(comms->_i2c_fd, I2C_RDWR, &msgset) < 0) {
      result = COMMS_RESULT_ERROR_READ_FAILED;
    }
  }
  return result;
}

comms_result_t comms_write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                               const uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  struct i2c_msg msgs[1];
  struct i2c_rdwr_ioctl_data msgset;
  uint8_t buf[1U + COMMS_BUFFER_SIZE];

  if ((comms == NULL) || (data == NULL) || (size == 0U) ||
      (size > COMMS_BUFFER_SIZE) || !comms->_initialized ||
      (comms->_i2c_fd < 0)) {
    result = COMMS_RESULT_ERROR_WRITE_FAILED;
  } else {
    buf[0] = reg;
    (void)memcpy(&buf[1], data, size);

    msgs[0].addr = address;
    msgs[0].flags = 0;
    msgs[0].len = (uint16_t)(1U + size);
    msgs[0].buf = buf;

    msgset.msgs = msgs;
    msgset.nmsgs = 1;

    if (ioctl(comms->_i2c_fd, I2C_RDWR, &msgset) < 0) {
      result = COMMS_RESULT_ERROR_WRITE_FAILED;
    }
  }
  return result;
}

#endif /* COMMS_BACKEND == COMMS_BACKEND_HARDWARE */
