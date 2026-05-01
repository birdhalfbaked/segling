#include "ahrs.h"
#include "comms.h"
#include "errors.h"
#include "storage.h"

static storage_t storage;
static ahrs_t ahrs;
static comms_t comms;

comms_result_t read_magnetometer(comms_t *comms,
                                 magnetometer_value_t *magnetometer_value) {
  uint8_t buffer[MAGNETOMETER_DATA_SIZE];
  comms_result_t result =
      read_i2c(comms, MAGNETOMETER_ADDRESS, MAGNETOMETER_START_REGISTER, buffer,
               MAGNETOMETER_DATA_SIZE);
  if (result == COMMS_RESULT_OK) {
    magnetometer_value->magnet_x = (int16_t)buffer[0] << 8 | buffer[1];
    magnetometer_value->magnet_y = (int16_t)buffer[2] << 8 | buffer[3];
    magnetometer_value->magnet_z = (int16_t)buffer[4] << 8 | buffer[5];
  }
  return result;
}

void update_ahrs(ahrs_t *ahrs, comms_t *comms) {
  // 1. read magnetometer data
  magnetometer_value_t magnetometer_value;
  comms_result_t read_result = read_magnetometer(comms, &magnetometer_value);
  if (read_result != COMMS_RESULT_OK) {
    ahrs->magnetometer_state = AHRS_STATE_FAILED;
  }
  // 2. update ahrs
  ahrs_result_t update_result =
      ahrs_update_magnetometer(ahrs, &magnetometer_value);
  if (update_result != AHRS_RESULT_OK) {
    ahrs->magnetometer_state = AHRS_STATE_FAILED;
  }
  // 3. save public sensor data to storage
  ahrs_public_t ahrs_public = ahrs_get_data(ahrs);
}

void deinit_system() { comms_deinit(&comms); }

int main(int argc, char *argv[]) {
  comms_result_t comms_result = comms_init(&comms);
  storage_result_t storage_result = storage_init(&storage);
  ahrs_result_t ahrs_result = ahrs_init(&ahrs);

  if (comms_result != COMMS_RESULT_OK || storage_result != STORAGE_RESULT_OK ||
      ahrs_result != AHRS_RESULT_OK) {
    deinit_system();
    return -1;
  }
  while (1) {
    update_ahrs(&ahrs, &comms);
  }
  comms_deinit(&comms);
  return 0;
}