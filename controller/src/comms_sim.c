/// @file comms_sim.c
/// @brief File-backed I2C simulation when COMMS_BACKEND == COMMS_BACKEND_SIM.

#include "controller/comms.h"

#if COMMS_BACKEND == COMMS_BACKEND_SIM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Match integrations/ahrs.h bus map; extend switch when adding sensors. */
#define COMMS_SIM_IMU_ADDRESS (0x68U)
#define COMMS_SIM_MAG_ADDRESS (0x0CU)

static bool sim_device_filename(uint8_t address, const char **filename_out) {
  bool found = true;
  if (filename_out == NULL) {
    found = false;
  } else {
    switch (address) {
    case COMMS_SIM_IMU_ADDRESS:
      *filename_out = "imu.dat";
      break;
    case COMMS_SIM_MAG_ADDRESS:
      *filename_out = "mag.dat";
      break;
    default:
      found = false;
      break;
    }
  }
  return found;
}

static comms_sim_record_t *sim_find_record(comms_t *comms, uint8_t address,
                                           uint8_t reg) {
  size_t index;
  comms_sim_record_t *result = NULL;

  for (index = 0U; index < comms->_sim_record_count; index++) {
    if ((comms->_sim_records[index].address == address) &&
        (comms->_sim_records[index].reg == reg)) {
      result = &comms->_sim_records[index];
      break;
    }
  }
  return result;
}

static comms_result_t sim_parse_device_line(comms_t *comms, uint8_t address,
                                            const char *line) {
  comms_result_t result = COMMS_RESULT_OK;
  unsigned int reg = 0U;
  unsigned int byte_val = 0U;
  int consumed = 0;
  int header_matched = 0;
  const char *cursor = NULL;
  comms_sim_record_t *record = NULL;

  if ((comms == NULL) || (line == NULL)) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else if ((line[0] == '#') || (line[0] == '\0') || (line[0] == '\n')) {
    result = COMMS_RESULT_OK;
  } else if (comms->_sim_record_count >= COMMS_SIM_MAX_RECORDS) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else {
    header_matched = sscanf(line, " %x%n", &reg, &consumed);
    if (header_matched < 1) {
      result = COMMS_RESULT_ERROR_UNKNOWN;
    } else {
      record = &comms->_sim_records[comms->_sim_record_count];
      record->address = address;
      record->reg = (uint8_t)reg;
      record->len = 0U;
      cursor = line + consumed;

      while (record->len < COMMS_SIM_MAX_DATA_LEN) {
        consumed = 0;
        if (sscanf(cursor, " %x%n", &byte_val, &consumed) != 1) {
          break;
        }
        record->data[record->len] = (uint8_t)byte_val;
        record->len++;
        cursor += consumed;
      }
      if (record->len == 0U) {
        result = COMMS_RESULT_ERROR_UNKNOWN;
      } else {
        comms->_sim_record_count++;
      }
    }
  }
  return result;
}

static comms_result_t sim_load_device_file(comms_t *comms, uint8_t address,
                                           const char *filename) {
  comms_result_t result = COMMS_RESULT_OK;
  FILE *file = NULL;
  char line[256];
  char path[512];

  if ((comms == NULL) || (filename == NULL)) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else if (snprintf(path, sizeof(path), "%s/%s", comms->_sim_data_dir,
                      filename) >= (int)sizeof(path)) {
    result = COMMS_RESULT_ERROR_UNKNOWN;
  } else {
    file = fopen(path, "r");
    if (file == NULL) {
      result = COMMS_RESULT_ERROR_READ_FAILED;
    } else {
      while (fgets(line, (int)sizeof(line), file) != NULL) {
        comms_result_t parse_result =
            sim_parse_device_line(comms, address, line);
        if (parse_result != COMMS_RESULT_OK) {
          result = parse_result;
          break;
        }
      }
      (void)fclose(file);
    }
  }
  return result;
}

static comms_result_t sim_ensure_device_loaded(comms_t *comms, uint8_t address) {
  comms_result_t result = COMMS_RESULT_OK;
  const char *filename = NULL;

  if ((comms == NULL) || !comms->_initialized) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else if (comms->_sim_addr_loaded[address]) {
    result = COMMS_RESULT_OK;
  } else if (!sim_device_filename(address, &filename)) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else {
    result = sim_load_device_file(comms, address, filename);
    if (result == COMMS_RESULT_OK) {
      comms->_sim_addr_loaded[address] = true;
    }
  }
  return result;
}

static comms_result_t sim_flush_device(comms_t *comms, uint8_t address) {
  comms_result_t result = COMMS_RESULT_OK;
  const char *filename = NULL;
  FILE *file = NULL;
  char path[512];
  size_t index;

  if ((comms == NULL) || !comms->_sim_dirty_addrs[address]) {
    result = COMMS_RESULT_OK;
  } else if (!sim_device_filename(address, &filename)) {
    result = COMMS_RESULT_ERROR_WRITE_FAILED;
  } else if (snprintf(path, sizeof(path), "%s/%s", comms->_sim_data_dir,
                      filename) >= (int)sizeof(path)) {
    result = COMMS_RESULT_ERROR_WRITE_FAILED;
  } else {
    file = fopen(path, "w");
    if (file == NULL) {
      result = COMMS_RESULT_ERROR_WRITE_FAILED;
    } else {
      (void)fprintf(file, "# addr %02x reg data_bytes...\n", (unsigned int)address);
      for (index = 0U; index < comms->_sim_record_count; index++) {
        const comms_sim_record_t *record = &comms->_sim_records[index];
        size_t byte_index;
        if (record->address != address) {
          continue;
        }
        (void)fprintf(file, "%02x", (unsigned int)record->reg);
        for (byte_index = 0U; byte_index < (size_t)record->len; byte_index++) {
          (void)fprintf(file, " %02x",
                        (unsigned int)record->data[byte_index]);
        }
        (void)fprintf(file, "\n");
      }
      (void)fclose(file);
      comms->_sim_dirty_addrs[address] = false;
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
    comms->_i2c_fd = -1;
    (void)strncpy(comms->_sim_data_dir, COMMS_SIM_DATA_DIR,
                  sizeof(comms->_sim_data_dir) - 1U);
    comms->_sim_data_dir[sizeof(comms->_sim_data_dir) - 1U] = '\0';
    comms->_initialized = true;
  }
  return result;
}

comms_result_t comms_deinit(comms_t *comms) {
  comms_result_t result = COMMS_RESULT_OK;
  uint16_t address;

  if ((comms != NULL) && comms->_initialized) {
    for (address = 0U; address < 256U; address++) {
      comms_result_t flush_result = sim_flush_device(comms, (uint8_t)address);
      if (flush_result != COMMS_RESULT_OK) {
        result = flush_result;
      }
    }
    comms->_initialized = false;
  }
  return result;
}

comms_result_t comms_read_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                              uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  const comms_sim_record_t *record = NULL;
  size_t copy_len;

  if ((comms == NULL) || (data == NULL) || (size == 0U) || !comms->_initialized) {
    result = COMMS_RESULT_ERROR_READ_FAILED;
  } else {
    result = sim_ensure_device_loaded(comms, address);
    if (result == COMMS_RESULT_OK) {
      record = sim_find_record(comms, address, reg);
      if (record == NULL) {
        result = COMMS_RESULT_ERROR_READ_FAILED;
      } else {
        copy_len = (size < (size_t)record->len) ? size : (size_t)record->len;
        (void)memcpy(data, record->data, copy_len);
        if (copy_len < size) {
          (void)memset(data + copy_len, 0, size - copy_len);
        }
      }
    }
  }
  return result;
}

comms_result_t comms_write_i2c(comms_t *comms, uint8_t address, uint8_t reg,
                               const uint8_t *data, size_t size) {
  comms_result_t result = COMMS_RESULT_OK;
  comms_sim_record_t *record = NULL;

  if ((comms == NULL) || (data == NULL) || (size == 0U) ||
      (size > COMMS_SIM_MAX_DATA_LEN) || !comms->_initialized) {
    result = COMMS_RESULT_ERROR_WRITE_FAILED;
  } else {
    result = sim_ensure_device_loaded(comms, address);
    if (result == COMMS_RESULT_OK) {
      record = sim_find_record(comms, address, reg);
      if (record == NULL) {
        if (comms->_sim_record_count >= COMMS_SIM_MAX_RECORDS) {
          result = COMMS_RESULT_ERROR_WRITE_FAILED;
        } else {
          record = &comms->_sim_records[comms->_sim_record_count];
          record->address = address;
          record->reg = reg;
          comms->_sim_record_count++;
        }
      }
      if (record != NULL) {
        record->len = (uint8_t)size;
        (void)memcpy(record->data, data, size);
        comms->_sim_dirty_addrs[address] = true;
      }
    }
  }
  return result;
}

#endif /* COMMS_BACKEND == COMMS_BACKEND_SIM */
