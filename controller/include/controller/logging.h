/// @defgroup LOGGING Logging module
/// @ingroup CORE
/// @{
#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>

#include "scheduling.h"

/// @defgroup LOGGING_types LOGGING types
/// @{

typedef enum {
  LOGGING_RESULT_OK = 0,
  LOGGING_RESULT_ERROR_UNKNOWN = -1,
  LOGGING_RESULT_ERROR_IO = -2,
} logging_result_t;

/// Maximum formatted message length (including NUL) for log_messagef scratch
/// buffer.
#define LOGGING_FORMAT_MAX 1024

/// Severity level for log records written to the events log.
typedef enum {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_ERROR = 3,
} log_level_t;

/// @}
/// @defgroup LOGGING_methods LOGGING methods
/// @{

/// Open (append) the events log file. Call once during application startup
/// (after success, log_message / log_messagef write here). Safe to call more
/// than once.
logging_result_t logging_init(void);

/// Close the events log file if open.
void logging_deinit(void);

/// Write one line: [epoch][slot_id][LEVEL] - message. No-op if logging_init did
/// not open the file.
void log_message(epoch_t epoch, slot_id_t slot_id, log_level_t level,
                 const char *message);

/// Write one line: [epoch][slot_id][LEVEL] - formatted message. No-op if
/// logging_init did not open the file.
void log_messagef(epoch_t epoch, slot_id_t slot_id, log_level_t level,
                  const char *format, ...);

/// @}
#endif
/// @}
