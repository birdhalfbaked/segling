#include "logging.h"
#include "controller.h"
#include <stdio.h>

#define LOG_EVENTS_PATH "/var/log/segling/events.log"

static FILE *g_events_log;

static const char *level_name(log_level_t level) {
  switch (level) {
  case LOG_LEVEL_DEBUG:
    return "DEBUG";
  case LOG_LEVEL_INFO:
    return "INFO";
  case LOG_LEVEL_WARN:
    return "WARN";
  case LOG_LEVEL_ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

logging_result_t logging_init(void) {
  if (g_events_log != NULL) {
    return LOGGING_RESULT_OK;
  }
  g_events_log = fopen(LOG_EVENTS_PATH, "a");
  if (g_events_log == NULL) {
    return LOGGING_RESULT_ERROR_IO;
  }
  (void)setvbuf(g_events_log, NULL, _IOLBF, 0);
  return LOGGING_RESULT_OK;
}

void logging_deinit(void) {
  if (g_events_log != NULL) {
    (void)fclose(g_events_log);
    g_events_log = NULL;
  }
}

void log_message(uint64_t tick, log_level_t level, const char *message) {
  if (g_events_log != NULL && level >= controller_config.log_level) {
    (void)fprintf(g_events_log, "[%llu][%s] - %s\n", tick, level_name(level),
                  message);
    (void)fflush(g_events_log);
  }
}