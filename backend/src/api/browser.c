
#include "../server.h"
#include "../api.h"

#include "../ts3remote.h"

static void mg_handler_get_browser_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/browser")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      struct TS3Browser *browser = &remote->browser;
      struct TS3Channel *channel = &browser->channel;
      struct TS3Channel *childs = &browser->childs[0];

      if (channel->name == NULL) {
        return mg_http_api_response(conn, "404 Not Found", NULL);
      }

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_json_chunk(conn, "%s", "{" HTTP_JSON_CHANNEL ",\"sub_channels\":[",
          mg_json_number(channel->id), mg_json_string(channel->name), mg_json_number(channel->order),
          mg_json_bool(channel->hasChannels), mg_json_bool(channel->hasPassword));

      for (int i = 0; i < browser->numChilds; i++) {
        mg_http_printf_json_chunk(conn, i ? ", %s" : "%s", "{" HTTP_JSON_CHANNEL "}",
            mg_json_number(childs[i].id), mg_json_string(childs[i].name), mg_json_number(childs[i].order),
            mg_json_bool(childs[i].hasChannels), mg_json_bool(childs[i].hasPassword));
      }

      mg_http_printf_chunk(conn, "]}");
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_browser() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_browser_fn);
  return &handler;
}

static void mg_handler_move_browser_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/browser/move")) {
      int channelId = 0;

      if (!mg_json_get_integer(msg->body, "$.channel_id", &channelId) || channelId < -1) {
        return mg_http_api_response(conn, "400 Bad Request", NULL);
      }

      struct TS3Remote *remote = TS3Remote_getInstance(0);
      TS3Remote_setBrowserToChannel(remote, (uint64) channelId);

      mg_http_api_response(conn, "200 OK", NULL);
    }
  }
}

struct mg_handler* mg_handler_move_browser() {
  static struct mg_handler handler = mg_handler_of(mg_handler_move_browser_fn);
  return &handler;
}
