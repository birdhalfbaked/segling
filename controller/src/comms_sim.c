/// @file comms_sim.c
/// @brief File-backed I2C simulation when COMMS_BACKEND == COMMS_BACKEND_SIM.

#include "controller/comms.h"

#if COMMS_BACKEND == COMMS_BACKEND_SIM

#include <stdio.h>
#include <string.h>

#ifndef COMMS_SIM_DATA_DIR
#define COMMS_SIM_DATA_DIR "simulated_data"
#endif

#define COMMS_SIM_MAX_FRAME_COUNT (255U)

/* Match integrations/ahrs.h bus map; extend switch when adding sensors. */
#define COMMS_SIM_IMU_ADDRESS (0x68U)
#define COMMS_SIM_IMU_START_REG (0x3BU)
#define COMMS_SIM_IMU_FRAME_LEN (14U)

#define COMMS_SIM_MAG_ADDRESS (0x0CU)
#define COMMS_SIM_MAG_START_REG (0x02U)
#define COMMS_SIM_MAG_FRAME_LEN (6U)

typedef struct {
  const char *filename;
  uint8_t start_reg;
  uint8_t frame_len;
} sim_device_desc_t;

typedef struct {
  bool loaded;
  bool disconnected;
  uint8_t start_reg;
  uint8_t frame_len;
  uint8_t frame_count;
  uint8_t line_index;
  FILE *file;
} sim_device_t;

static char g_sim_data_dir[256];
static sim_device_t g_sim_devices[256];

static bool sim_device_desc(uint8_t address, sim_device_desc_t *desc_out) {
  bool found = true;

  if (desc_out == NULL) {
    found = false;
  } else {
    switch (address) {
    case COMMS_SIM_IMU_ADDRESS:
      desc_out->filename = "imu.dat";
      desc_out->start_reg = COMMS_SIM_IMU_START_REG;
      desc_out->frame_len = COMMS_SIM_IMU_FRAME_LEN;
      break;
    case COMMS_SIM_MAG_ADDRESS:
      desc_out->filename = "mag.dat";
      desc_out->start_reg = COMMS_SIM_MAG_START_REG;
      desc_out->frame_len = COMMS_SIM_MAG_FRAME_LEN;
      break;
    default:
      found = false;
      break;
    }
  }
  return found;
}

static comms_result_t sim_close_device(sim_device_t *device) {
  if ((device != NULL) && (device->file != NULL)) {
    (void)fclose(device->file);
    device->file = NULL;
  }
  if (device != NULL) {
    device->loaded = false;
  }
  return COMMS_RESULT_OK;
}

static comms_result_t sim_load_device(uint8_t address) {
  comms_result_t result = COMMS_RESULT_OK;
  sim_device_desc_t desc;
  sim_device_t *device = &g_sim_devices[address];
  char path[512];
  long file_size = 0L;
  long frame_count_long = 0L;

  if (!sim_device_desc(address, &desc)) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else if (device->loaded) {
    result = COMMS_RESULT_OK;
  } else if (snprintf(path, sizeof(path), "%s/%s", g_sim_data_dir,
                      desc.filename) >= (int)sizeof(path)) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else {
    (void)memset(device, 0, sizeof(*device));
    device->file = fopen(path, "rb+");
    if (device->file == NULL) {
      result = COMMS_RESULT_ERROR_READ_FAILED;
    } else {
      device->start_reg = desc.start_reg;
      device->frame_len = desc.frame_len;
      if (fseek(device->file, 0L, SEEK_END) != 0) {
        result = COMMS_RESULT_ERROR_UNKNOWN;
      } else {
        file_size = ftell(device->file);
        if ((file_size < 0L) || ((file_size % (long)desc.frame_len) != 0L)) {
          result = COMMS_RESULT_ERROR_UNKNOWN;
        } else {
          frame_count_long = file_size / (long)desc.frame_len;
          if ((frame_count_long == 0L) ||
              (frame_count_long > (long)COMMS_SIM_MAX_FRAME_COUNT)) {
            result = COMMS_RESULT_ERROR_READ_FAILED;
          } else {
            device->frame_count = (uint8_t)frame_count_long;
            device->line_index = 0U;
            device->disconnected = false;
            device->loaded = true;
          }
        }
      }
      if (result != COMMS_RESULT_OK) {
        (void)sim_close_device(device);
      }
    }
  }
  return result;
}

static comms_result_t sim_seek_frame_reg(const sim_device_t *device, uint8_t reg) {
  comms_result_t result = COMMS_RESULT_OK;
  long offset = 0L;

  if ((device == NULL) || (device->file == NULL)) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else if (reg < device->start_reg) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else {
    offset =
        ((long)device->line_index * (long)device->frame_len) +
        (long)(reg - device->start_reg);
    if (fseek(device->file, offset, SEEK_SET) != 0) {
      result = COMMS_RESULT_ERROR_READ_FAILED;
    }
  }
  return result;
}

static comms_result_t sim_ensure_device(uint8_t address, sim_device_t **device_out) {
  comms_result_t result = COMMS_RESULT_OK;
  sim_device_t *device = &g_sim_devices[address];

  if (device_out == NULL) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else {
    result = sim_load_device(address);
    if (result == COMMS_RESULT_OK) {
      *device_out = device;
    }
  }
  return result;
}

comms_result_t comms_init(comms_t *comms) {
  comms_result_t result = COMMS_RESULT_OK;

  if (comms == NULL) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else {
    (void)memset(comms, 0, sizeof(*comms));
    (void)memset(g_sim_devices, 0, sizeof(g_sim_devices));
    comms->_i2c_fd = -1;
    (void)strncpy(g_sim_data_dir, COMMS_SIM_DATA_DIR, sizeof(g_sim_data_dir) - 1U);
    g_sim_data_dir[sizeof(g_sim_data_dir) - 1U] = '\0';
    comms->_initialized = true;
  }
  return result;
}

comms_result_t comms_deinit(comms_t *comms) {
  comms_result_t result = COMMS_RESULT_OK;
  uint16_t address;

  if ((comms != NULL) && comms->_initialized) {
    for (address = 0U; address < 256U; address++) {
      if (g_sim_devices[address].loaded) {
        (void)sim_close_device(&g_sim_devices[address]);
      }
    }
    comms->_initialized = false;
    comms->_i2c_fd = -1;
  }
  return result;
}

comms_result_t comms_read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                              uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  sim_device_t *device = NULL;
  size_t offset = 0U;

  if ((comms == NULL) || (data == NULL) || (size == 0U) || !comms->_initialized) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else {
    result = sim_ensure_device(address, &device);
    if (result == COMMS_RESULT_OK) {
      if ((device == NULL) || device->disconnected) {
        result = COMMS_RESULT_ERROR_READ_FAILED;
      } else if (device->line_index >= device->frame_count) {
        device->disconnected = true;
        result = COMMS_RESULT_ERROR_READ_FAILED;
      } else if (reg < device->start_reg) {
        result = COMMS_RESULT_ERROR_READ_FAILED;
      } else {
        offset = (size_t)(reg - device->start_reg);
        if (((offset + size) > (size_t)device->frame_len) ||
            (size > COMMS_BUFFER_SIZE)) {
          result = COMMS_RESULT_ERROR_READ_FAILED;
        } else {
          result = sim_seek_frame_reg(device, reg);
          if (result == COMMS_RESULT_OK) {
            if (fread(data, 1U, size, device->file) != size) {
              result = COMMS_RESULT_ERROR_READ_FAILED;
            } else {
              device->line_index++;
              if (device->line_index >= device->frame_count) {
                device->disconnected = true;
              }
            }
          }
        }
      }
    }
  }
  return result;
}

comms_result_t comms_write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                               const uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  sim_device_t *device = NULL;
  size_t offset = 0U;

  if ((comms == NULL) || (data == NULL) || (size == 0U) || !comms->_initialized) {
    result = COMMS_RESULT_ERROR_WRITE_FAILED;
  } else {
    result = sim_ensure_device(address, &device);
    if (result == COMMS_RESULT_OK) {
      if ((device == NULL) || device->disconnected) {
        result = COMMS_RESULT_ERROR_WRITE_FAILED;
      } else if (device->line_index >= device->frame_count) {
        device->disconnected = true;
        result = COMMS_RESULT_ERROR_WRITE_FAILED;
      } else if (reg < device->start_reg) {
        result = COMMS_RESULT_ERROR_WRITE_FAILED;
      } else {
        offset = (size_t)(reg - device->start_reg);
        if (((offset + size) > (size_t)device->frame_len) ||
            (size > COMMS_BUFFER_SIZE)) {
          result = COMMS_RESULT_ERROR_WRITE_FAILED;
        } else {
          result = sim_seek_frame_reg(device, reg);
          if (result == COMMS_RESULT_OK) {
            if (fwrite(data, 1U, size, device->file) != size) {
              result = COMMS_RESULT_ERROR_WRITE_FAILED;
            }
          }
        }
      }
    }
  }
  return result;
}

#endif /* COMMS_BACKEND == COMMS_BACKEND_SIM */
