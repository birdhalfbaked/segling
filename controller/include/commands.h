#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  COMMANDS_RESULT_OK = 0,
  COMMANDS_RESULT_ERROR_NO_COMMAND = -1,
  COMMANDS_RESULT_ERROR_UNKNOWN = -2,
  COMMANDS_RESULT_ERROR_FULL = -3,
  COMMANDS_RESULT_ERROR_INVALID_COMMAND = -4,
  COMMANDS_RESULT_ERROR_INVALID_ARGS = -5,
} commands_result_t;

typedef enum {
  COMMAND_CALIBRATE_IMU = 1,
  COMMAND_CALIBRATE_COMPASS = 2,
} command_type_t;

/// Command structure
/// @brief Command structure
/// @param type Command type
/// @param id Command ID - we must not have more than 256 commands awaiting
/// execution
typedef struct {
  command_type_t type;
  uint64_t id;
} command_t;

typedef struct {
  command_t arr[256];
  uint64_t last_handled_id;
  size_t pointer;
} command_list_t;

/// @defgroup COMMANDS_methods COMMANDS methods
/// @{

/// Add a command to the command list
/// @brief Add a command to the command list
/// @param commands Command list
/// @param command Command to add
/// @pre commands->last_handled_id < command.id
/// @pre command.id - commands->last_handled_id <= 256
/// @return COMMANDS_RESULT_OK if the command was added,
/// COMMANDS_RESULT_ERROR_FULL if the command list is full,
/// COMMANDS_RESULT_ERROR_INVALID_COMMAND if the command is invalid or ID is out
/// of order
commands_result_t commands_add(command_list_t *commands, command_t command);

/// Execute the commands in the command list
/// @brief Execute the commands in the command list
/// @param commands Command list
/// @return COMMANDS_RESULT_OK if the commands were executed,
/// COMMANDS_RESULT_ERROR_UNKNOWN if an error occurred
commands_result_t commands_execute(command_list_t *commands);
/// @}

#endif
