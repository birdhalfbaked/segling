#ifndef COMMS_H
#define COMMS_H

#include "errors.h"
#include <stddef.h>
#include <stdint.h>

#define I2C_DEVICE "/dev/i2c-1"

typedef struct {
  int i2c_fd;
  int serial_fd;
} comms_t;

comms_result_t comms_init(comms_t *comms);
comms_result_t comms_deinit(comms_t *comms);

comms_result_t read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                        uint8_t *data, size_t size);
comms_result_t write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                         uint8_t *data, size_t size);

#endif