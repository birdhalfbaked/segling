#include "comms.h"
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

comms_result_t comms_init(comms_t *comms) {
  comms->i2c_fd = open(I2C_DEVICE, O_RDWR);
  if (comms->i2c_fd == -1) {
    return COMMS_RESULT_ERROR_READ_FAILED;
  }
  return COMMS_RESULT_OK;
}

comms_result_t comms_deinit(comms_t *comms) {
  close(comms->i2c_fd);
  return COMMS_RESULT_OK;
}

comms_result_t read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                        uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  struct i2c_msg msgs[2];
  struct i2c_rdwr_ioctl_data msgset;

  msgs[0].addr = address;
  msgs[0].flags = 0;
  msgs[0].len = 1;
  msgs[0].buf = &reg;

  msgs[1].addr = address;
  msgs[1].flags = I2C_M_RD;
  msgs[1].len = size;
  msgs[1].buf = data;

  msgset.msgs = msgs;
  msgset.nmsgs = 2;

  if (ioctl(comms->i2c_fd, I2C_RDWR, &msgset) < 0) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  }
  return result;
}

comms_result_t write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                         uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  struct i2c_msg msgs[1];
  struct i2c_rdwr_ioctl_data msgset;

  msgs[0].addr = address;
  msgs[0].flags = 0;
  msgs[0].len = 1;
  msgs[0].buf = &reg;
  msgs[0].buf = data;
  msgset.msgs = msgs;
  msgset.nmsgs = 1;
  if (ioctl(comms->i2c_fd, I2C_RDWR, &msgset) < 0) {
    result = COMMS_RESULT_ERROR_WRITE_FAILED;
  }
  return result;
}