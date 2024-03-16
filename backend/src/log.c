
#include "plugin.h"
#include "log.h"

static void Logger_log(enum LogLevel severity, const char *format, va_list arguments) {
  char message[512];
  vsnprintf(message, sizeof(message), format, arguments);

  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  if (ts3->logMessage != NULL) {
    ts3->logMessage(message, severity, "ts3-qs4sd", 0);
  } else {
    printf("%s\n", message);
  }
}

#define Logger_logMessage(severity, message) do { \
  va_list arguments; \
  va_start(arguments, message); \
  Logger_log(severity, message, arguments); \
  va_end(arguments); \
} while (0);

void Logger_debugLog(const char *message, ...) {
  Logger_logMessage(LogLevel_DEBUG, message);
}

void Logger_infoLog(const char *message, ...) {
  Logger_logMessage(LogLevel_INFO, message);
}

void Logger_warnLog(const char *message, ...) {
  Logger_logMessage(LogLevel_WARNING, message);
}

void Logger_errorLog(const char *message, ...) {
  Logger_logMessage(LogLevel_ERROR, message);
}
