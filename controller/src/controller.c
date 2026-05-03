#include "controller.h"
#include "ahrs.h"
#include "commands.h"
#include "comms.h"
#include "storage.h"

controller_config_t controller_config = {
    .log_level = LOG_LEVEL_INFO, // default to info
};

controller_result_t controller_init(controller_t *controller) {
  logging_init();
  controller_result_t result = CONTROLLER_RESULT_OK;
  ahrs_result_t ahrs_result = ahrs_init(&controller->ahrs);
  if (ahrs_result != AHRS_RESULT_OK) {
    result = CONTROLLER_RESULT_ERROR_INIT_FAILED;
  }
  if (result == CONTROLLER_RESULT_OK) {
    comms_result_t comms_result = comms_init(&controller->comms);
    if (comms_result != COMMS_RESULT_OK) {
      result = CONTROLLER_RESULT_ERROR_INIT_FAILED;
    }
  }
  if (result == CONTROLLER_RESULT_OK) {
    storage_result_t storage_result = storage_init(&controller->storage);
    if (storage_result != STORAGE_RESULT_OK) {
      result = CONTROLLER_RESULT_ERROR_INIT_FAILED;
    }
  }
  if (result == CONTROLLER_RESULT_OK) {
    controller->tick = 0;
    controller->commands.last_handled_id = 0;
    controller->commands.pointer = 0;
  }
  return result;
}

controller_result_t controller_deinit(controller_t *controller) {
  controller_result_t result = CONTROLLER_RESULT_OK;
  comms_result_t comms_result = comms_deinit(&controller->comms);
  if (comms_result != COMMS_RESULT_OK) {
    result = CONTROLLER_RESULT_ERROR_DEINIT_FAILED;
  }
  if (result == CONTROLLER_RESULT_OK) {
    storage_result_t storage_result = storage_deinit(&controller->storage);
    if (storage_result != STORAGE_RESULT_OK) {
      result = CONTROLLER_RESULT_ERROR_DEINIT_FAILED;
    }
  }
  return result;
}