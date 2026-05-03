#include "commands.h"

commands_result_t commands_add(command_list_t *commands, command_t command) {
  commands_result_t result = COMMANDS_RESULT_OK;
  if (command.id <= commands->last_handled_id) {
    result = COMMANDS_RESULT_ERROR_INVALID_COMMAND;
  }
  if (result == COMMANDS_RESULT_OK &&
      command.id - commands->last_handled_id > 256) {
    result = COMMANDS_RESULT_ERROR_FULL;
  }
  if (result == COMMANDS_RESULT_OK) {
    commands->arr[commands->pointer] = command;
    // keep it like this, yes overflow in unsigned integer is defined behavior
    // but I'd rather be explicit the compiler will optimize it away anyway
    commands->pointer = (commands->pointer + 1) % 256;
  }
  return result;
}

commands_result_t commands_get_next(command_list_t *commands,
                                    command_t *command) {
  commands_result_t result = COMMANDS_RESULT_OK;
  if (command == NULL) {
    result = COMMANDS_RESULT_ERROR_INVALID_ARGS;
  }
  if (result == COMMANDS_RESULT_OK &&
      commands->arr[commands->pointer].id <= commands->last_handled_id) {
    result = COMMANDS_RESULT_ERROR_NO_COMMAND;
  }
  if (result == COMMANDS_RESULT_OK) {
    *command = commands->arr[commands->pointer];
  }
  return result;
}