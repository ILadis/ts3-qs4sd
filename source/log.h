#ifndef LOG_H
#define LOG_H

#include <stdio.h>

void Logger_debugLog(const char *message, ...);
void Logger_infoLog(const char *message, ...);
void Logger_warnLog(const char *message, ...);
void Logger_errorLog(const char *message, ...);
void Logger_enableDevToolsLogging(const char *url);

#endif
