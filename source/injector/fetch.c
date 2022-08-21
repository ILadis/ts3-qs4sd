
#include "states.h"

struct DevToolsPageItem {
  char title[100];
  char webSocketDebuggerUrl[250];
};

static int Injector_nextDevToolsPageItem(
    int event, const char *json,
    int off, int len, void *data)
{
  static struct Context {
    const char *key;
    int len, off;
    int depth;
  } context = {0};

  #define Context_hasKey(k) (strncmp("\"" k "\"", context.key, context.len) == 0)

  static struct DevToolsPageItem item = {0};
  const char *value = json + off;

  switch (event) {
  case MJSON_TOK_KEY:
    context.key = value;
    context.len = len;
    break;
  case MJSON_TOK_OBJECT:
  case MJSON_TOK_ARRAY:
    context.depth++;
    break;
  case MJSON_TOK_STRING:
    char *target = NULL;
    int size = 0;

    if (Context_hasKey("title")) {
      target = item.title;
      size = sizeof(item.title);
    }
    else if (Context_hasKey("webSocketDebuggerUrl")) {
      target = item.webSocketDebuggerUrl;
      size = sizeof(item.webSocketDebuggerUrl);
    }
    else return 0;

    int limit = len - 2;
    if (limit > size) {
      limit = size;
    }

    snprintf(target, limit, "%s", value + 1);
    break;
  case MJSON_TOK_OBJECT + 2:
  case MJSON_TOK_ARRAY + 2:
    int depth = --context.depth;
    void **retval = (void **) data;

    /* Return current item when:
     * - leaving current object and
     * - offset is larger than last time
     */
    if (off > context.off) {
      context.off = off;
      context.depth = 0;
      *retval = &item;

      // reached end of json document
      if (depth == 0) {
        context.key = NULL;
        context.len = context.off = 0;
        *retval = NULL;
      }

      return 1;
    }
  }

  return 0;
}

static void Injector_fetchWSUrlFn(
    struct mg_connection *conn,
    int event, void *data, void *context)
{
  if (event == MG_EV_CONNECT) {
    mg_printf(conn, HTTP_REQUEST(SETTINGS_ADDR(STEAM_DECK), "GET", "/json"));
    mg_printf(conn, HTTP_NEWLINE);
  }
  else if (event == MG_EV_HTTP_MSG) {
    struct Injector *injector = (struct Injector *) context;
    struct mg_http_message *msg = (struct mg_http_message *) data;

    struct DevToolsPageItem *item = NULL;
    while (mjson(msg->body.ptr, msg->body.len, Injector_nextDevToolsPageItem, &item) && item != NULL) {
      if (strcmp("QuickAccess", item->title) == 0) {
        snprintf(injector->wsUrl, sizeof(injector->wsUrl), "%s", item->webSocketDebuggerUrl);
        break;
      }
    }

    conn->is_closing = 1;

    if (item != NULL) {
      return Injector_gotoEvaluateJSCodeState(injector);
    } else {
      return Injector_gotoRetryState(injector);
    }
  }
}

void Injector_gotoFetchWSUrlState(struct Injector *injector) {
  const char url[] = "http://" SETTINGS_ADDR(STEAM_DECK) "/json";

  injector->state = STATE_FETCH_WS_URL;
  injector->conn = mg_http_connect(injector->manager, url, Injector_fetchWSUrlFn, injector);
}
