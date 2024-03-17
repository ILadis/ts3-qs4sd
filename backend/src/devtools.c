

#include "devtools.h"
#include "plugin.h"
#include "server.h"
#include "log.h"

static void DevTools_fetchDebuggerWsUrl(struct DevTools *dev);
static void DevTools_retryFetchDebuggerWsUrl(struct DevTools *dev);
static void DevTools_listenDebuggerWsUrl(struct DevTools *dev);

void DevTools_inspectTab(
    struct DevTools *dev,
    const char *url, const char *tab)
{
  snprintf(dev->url, sizeof(dev->url), "%s", url);
  snprintf(dev->tab, sizeof(dev->tab), "%s", tab);
  snprintf(dev->debuggerWsUrl, sizeof(dev->debuggerWsUrl), "%s", "");

  DevTools_fetchDebuggerWsUrl(dev);
}

bool DevTools_evaluateJSCode(
    struct DevTools *dev,
    const char *code)
{
  char message[1024];
  const char json[] = "{"
      "\"id\":%d,"
      "\"method\":\"Runtime.evaluate\","
      "\"params\":{"
          "\"expression\":%m,"
          "\"userGesture\":true,"
          "\"awaitPromise\":true"
      "}}";

  if (dev->conn == NULL) {
    return false;
  }

  size_t length = mg_snprintf(message, sizeof(message), json, mg_json_number(1), mg_json_string(code));
  mg_ws_send(dev->conn, message, length, WEBSOCKET_OP_TEXT);

  return true;
}

struct DevToolsPageItem {
  char title[100];
  char debuggerWsUrl[250];
};

static bool DevTools_nextPageItem(struct mg_str json, size_t *offset, struct DevToolsPageItem *item) {
  struct mg_str value;
  size_t next = *offset = mg_json_next(json, *offset, NULL, &value);

  if (next > 0) {
    mg_json_get_string(value, "$.title", item->title, sizeof(item->title));
    mg_json_get_string(value, "$.webSocketDebuggerUrl", item->debuggerWsUrl, sizeof(item->debuggerWsUrl));
    return true;
  }

  return false;
}

static void DevTools_fetchDebuggerWsUrlFn(
    struct mg_connection *conn,
    int event, void *data)
{
  struct DevTools *dev = (struct DevTools *) conn->fn_data;

  if (event == MG_EV_CONNECT) {
    struct mg_str host = mg_url_host(dev->url);
    unsigned short port = mg_url_port(dev->url);

    mg_printf(conn, "GET /json HTTP/1.1\r\nHost: %.*s:%hu\r\n", host.len, host.ptr, port);
    mg_printf(conn, "\r\n");
  }
  else if (event == MG_EV_ERROR) {
    Logger_errorLog("Could not fetch debugger ws url: %s", data);
    conn->is_closing = 1;
    return DevTools_retryFetchDebuggerWsUrl(dev);
  }
  else if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;
    struct DevToolsPageItem item = {0};

    size_t offset = 0;
    while (DevTools_nextPageItem(msg->body, &offset, &item)) {
      if (strncmp(dev->tab, item.title, sizeof(dev->tab)) == 0) {
        snprintf(dev->debuggerWsUrl, sizeof(dev->debuggerWsUrl), "%s", item.debuggerWsUrl);
        break;
      }
    }

    conn->is_closing = 1;

    bool result = dev->debuggerWsUrl[0] != '\0';
    if (result) {
      return DevTools_listenDebuggerWsUrl(dev);
    } else {
      Logger_errorLog("Could not find tab with name '%s' on: %s", dev->tab, dev->url);
      return DevTools_retryFetchDebuggerWsUrl(dev);
    }
  }
}

static void DevTools_fetchDebuggerWsUrl(struct DevTools *dev) {
  Logger_debugLog("Attempting to fetch debugger url for tab '%s' on: %s", dev->tab, dev->url);
  mg_http_connect(dev->manager, dev->url, DevTools_fetchDebuggerWsUrlFn, dev);
}

static void DevTools_printLogMessage(struct mg_ws_message *msg) {
  const struct mg_str methods[] = {
    mg_str("\"Log.entryAdded\""),
    mg_str("\"Runtime.consoleAPICalled\""),
    mg_str("\"Runtime.exceptionThrown\""),
  };

  struct mg_str method, params;
  if (mg_json_find(msg->data, "$.method", &method)) {
    for (int i = 0; i < length(methods); i++) {
      if (mg_strcmp(methods[i], method) == 0) {
        if (mg_json_find(msg->data, "$.params", &params)) {
          Logger_debugLog("DevTools (%.*s): %.*s", method.len, method.ptr, params.len, params.ptr);
        }
      }
    }
  }
}

static void DevTools_listenDebuggerWsUrlFn(
    struct mg_connection *conn,
    int event, void *data)
{
  struct DevTools *dev = (struct DevTools *) conn->fn_data;

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
      DevTools_printLogMessage(msg);
    }
  }
  else if (event == MG_EV_ERROR || event == MG_EV_CLOSE) {
    dev->conn = NULL;

    Logger_debugLog("Disconnected from dev tools (error or close)");
    DevTools_retryFetchDebuggerWsUrl(dev);
  }
}

static void DevTools_listenDebuggerWsUrl(struct DevTools *dev) {
  Logger_debugLog("Connecting to debugger url: %s", dev->debuggerWsUrl);
  dev->conn = mg_ws_connect(dev->manager, dev->debuggerWsUrl, DevTools_listenDebuggerWsUrlFn, dev, NULL);
}

static void DevTools_retryFetchDebuggerWsUrlFn(void *context) {
  struct DevTools *dev = (struct DevTools *) context;
  Logger_debugLog("Retrying to fetch debugger url for tab '%s' on: %s", dev->tab, dev->url);
  mg_http_connect(dev->manager, dev->url, DevTools_fetchDebuggerWsUrlFn, dev);
}

static void DevTools_retryFetchDebuggerWsUrl(struct DevTools *dev) {
  const int timer = 3000;
  Logger_debugLog("Retrying to connect to dev tools after %ld ms...", timer);
  mg_timer_add(dev->manager, timer, 0, DevTools_retryFetchDebuggerWsUrlFn, dev);
}
