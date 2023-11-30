
#include "../server.h"
#include "../api.h"

#include "../plugin.h"
#include "../ts3remote.h"

static void mg_handler_get_cursor_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/cursor")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      struct TS3Cursor *cursor = &remote->cursor;
      struct TS3Channel *channel = &cursor->channel;
      struct TS3Client *clients = &cursor->clients[0];

      if (channel->name == NULL) {
        return mg_http_api_response(conn, "404 Not Found", NULL);
      }

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_json_chunk(conn, "%s", "{" HTTP_JSON_CHANNEL ",\"clients\":[",
        channel->id, channel->name, channel->order, channel->maxClients, channel->hasPassword);

      for (int i = 0; i < cursor->numClients; i++) {
        mg_http_printf_json_chunk(conn, i ? ", %s" : "%s", "{" HTTP_JSON_CLIENT "}",
          clients[i].id, clients[i].nickname, clients[i].inputMuted, clients[i].outputMuted);
      }

      mg_http_printf_chunk(conn, "]}");
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_cursor() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_cursor_fn);
  return &handler;
}

static void mg_handler_move_cursor_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/cursor/move")) {
      int channelId = 0;

      if (!mg_http_get_json_integer(msg, "$.channel_id", &channelId) || channelId < 0) {
        return mg_http_api_response(conn, "400 Bad Request", NULL);
      }

      struct TS3Remote *remote = TS3Remote_getInstance(0);
      TS3Remote_setCursorToChannel(remote, (uint64) channelId);

      return mg_http_api_response(conn, "200 OK", NULL);
    }
  }
}

struct mg_handler* mg_handler_move_cursor() {
  static struct mg_handler handler = mg_handler_of(mg_handler_move_cursor_fn);
  return &handler;
}


static void mg_handler_join_cursor_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/cursor/join")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      bool result = TS3Remote_joinCursor(remote);

      return mg_http_api_response(conn, result ? "200 OK" : "500 Internal Server Error", NULL);
    }
  }
}

struct mg_handler* mg_handler_join_cursor() {
  static struct mg_handler handler = mg_handler_of(mg_handler_join_cursor_fn);
  return &handler;
}
