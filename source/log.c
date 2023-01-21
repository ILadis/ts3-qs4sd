
#include "plugin.h"
#include "server.h"
#include "log.h"

static void Logger_log(enum LogLevel severity, const char *format, va_list arguments) {
  char message[512];
  vsnprintf(message, sizeof(message), format, arguments);

  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  ts3->logMessage(message, severity, "ts3-qs4sd", 0);
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

static void Logger_devToolsLog(struct mg_ws_message *msg) {
  const char *methods[] = {
    "Log.entryAdded",
    "Runtime.consoleAPICalled",
    "Runtime.exceptionThrown",
    NULL,
  };

  char method[64];
  if (mg_ws_get_json_string(msg, "$.method", method, sizeof(method))) {
    for (int i = 0; methods[i]; i++) {
      if (strcmp(methods[i], method) == 0) {
        const char *params; int len;
        if (mjson_find(msg->data.ptr, msg->data.len, "$.params", &params, &len)) {
          Logger_debugLog("DevTools (%s): %.*s", methods[i], len, params);
        }
      }
    }
  }
}

static void Logger_devToolsLogFn(
    struct mg_connection *conn,
    int event, void *data, void *context)
{
  if (event == MG_EV_WS_OPEN) {
    const char enableLog[] = "{\"id\":1, \"method\":\"Log.enable\"}";
    mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "%s", enableLog);

    const char enableRuntime[] = "{\"id\":2, \"method\":\"Runtime.enable\"}";
    mg_ws_printf(conn, WEBSOCKET_OP_TEXT, "%s", enableRuntime);
  }
  else if (event == MG_EV_WS_MSG) {
    struct mg_ws_message *msg = (struct mg_ws_message *) data;
    int type = msg->flags & 0x0F;
    if (type == WEBSOCKET_OP_TEXT) {
      Logger_devToolsLog(msg);
    }
  }
  else if (event == MG_EV_ERROR || event == MG_EV_CLOSE) {
    *((struct mg_connection **) context) = NULL;
    Logger_debugLog("Stopping DevTools logging (error or close)");
  }
}

void Logger_enableDevToolsLogging(const char *url) {
  struct mg_server *server = ts3plugin_getServer();
  struct mg_mgr *manager = &server->manager;

  static struct mg_connection *conn = NULL;

  if (conn == NULL) {
    Logger_debugLog("Listening to DevTools log on: %s", url);
    conn = mg_ws_connect(manager, url, Logger_devToolsLogFn, &conn, NULL);
  }
}
