
#include "../server.h"
#include "../api.h"

#include "../plugin.h"

static const char *tag = "EVENTS";

static inline void mg_handler_events_set_tag(struct mg_connection *conn) {
  strcpy(conn->data, tag);
}

static inline bool mg_handler_events_is_tagged(struct mg_connection *conn) {
  return strcmp(conn->data, tag) == 0;
}

static void mg_handler_events_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/events")) {
      mg_handler_events_set_tag(conn);
      mg_http_api_response(conn, "200 OK", "text/event-stream");
    }
  }
  else if (event >= USER_EVENT && mg_handler_events_is_tagged(conn)) {
    const char *type = NULL;

    switch (event) {
    case CONNECTION_STATE_CONNECTED:
      type = "CONNECTION_STATE_CONNECTED";
      break;
    case CONNECTION_STATE_DISCONNECTED:
      type = "CONNECTION_STATE_DISCONNECTED";
      break;
    case CLIENT_LIST_CHANGED:
      type = "CLIENT_LIST_CHANGED";
      break;
    case AUDIO_OUTPUTS_CHANGED:
      type = "AUDIO_OUTPUTS_CHANGED";
      break;
    }

    if (type != NULL) {
      return mg_http_printf_sse_json_chunk(conn, "{" HTTP_JSON_EVENT "}", type);
    }
  }
}


struct mg_handler* mg_handler_events() {
  static struct mg_handler handler = mg_handler_of(mg_handler_events_fn);
  return &handler;
}
