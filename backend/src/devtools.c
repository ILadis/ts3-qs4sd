

#include "devtools.h"
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
          "\"expression\":%Q,"
          "\"userGesture\":true,"
          "\"awaitPromise\":true"
      "}}";

  if (dev->conn == NULL) {
    return false;
  }

  int length = mjson_snprintf(message, sizeof(message), json, 1, code);
  mg_ws_send(dev->conn, message, length, WEBSOCKET_OP_TEXT);

  return true;
}

struct DevToolsPageItem {
  char title[100];
  char debuggerWsUrl[250];
  struct {
    const char *key;
    int length, offset;
    int depth;
  } context;
};

static bool DevToolsPageItem_contextHasKey(struct DevToolsPageItem *item, char *key) {
  return strncmp(key, item->context.key, item->context.length) == 0;
}

static int DevTools_nextPageItem(
    int event, const char *json,
    int offset, int length, void *data)
{
  struct DevToolsPageItem *item = (struct DevToolsPageItem *) data;
  const char *value = json + offset + 1; length -= 2;

  switch (event) {
  case MJSON_TOK_KEY:
    item->context.key = value;
    item->context.length = length;
    break;
  case MJSON_TOK_OBJECT:
  case MJSON_TOK_ARRAY:
    item->context.depth++;
    break;
  case MJSON_TOK_STRING:
    char *target = NULL;
    int size = 0;

    if (DevToolsPageItem_contextHasKey(item, "title")) {
      target = item->title;
      size = sizeof(item->title);
    }
    else if (DevToolsPageItem_contextHasKey(item, "webSocketDebuggerUrl")) {
      target = item->debuggerWsUrl;
      size = sizeof(item->debuggerWsUrl);
    }
    else return 0;

    snprintf(target, size, "%.*s", length, value);
    break;
  case MJSON_TOK_OBJECT + 2:
  case MJSON_TOK_ARRAY + 2:
    int depth = --item->context.depth;

    /* Return current item when:
     * - leaving current object and
     * - offset is larger than last time
     */
    if (offset > item->context.offset) {
      item->context.offset = offset;
      item->context.depth = 0;

      // reached end of json document
      if (depth == 0) {
        return 0;
      }

      return 1;
    }
  }

  return 0;
}

static void DevTools_fetchDebuggerWsUrlFn(
    struct mg_connection *conn,
    int event, void *data, void *context)
{
  struct DevTools *dev = (struct DevTools *) context;

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

    while (mjson(msg->body.ptr, msg->body.len, DevTools_nextPageItem, &item) > 0) {
      if (strncmp(dev->tab, item.title, sizeof(dev->tab)) == 0) {
        snprintf(dev->debuggerWsUrl, sizeof(dev->debuggerWsUrl), "%s", item.debuggerWsUrl);
        break;
      }
    }

    conn->is_closing = 1;

    const int result = strnlen(dev->debuggerWsUrl, sizeof(dev->debuggerWsUrl));
    if (result <= 0) {
      Logger_errorLog("Could not find tab with name '%s' on: %s", dev->tab, dev->url);
      return DevTools_retryFetchDebuggerWsUrl(dev);
    } else {
      return DevTools_listenDebuggerWsUrl(dev);
    }
  }
}

static void DevTools_fetchDebuggerWsUrl(struct DevTools *dev) {
  Logger_debugLog("Attempting to fetch debugger url for tab '%s' on: %s", dev->tab, dev->url);
  mg_http_connect(dev->manager, dev->url, DevTools_fetchDebuggerWsUrlFn, dev);
}

static void DevTools_printLogMessage(struct mg_ws_message *msg) {
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

static void DevTools_listenDebuggerWsUrlFn(
    struct mg_connection *conn,
    int event, void *data, void *context)
{
  struct DevTools *dev = (struct DevTools *) context;

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
