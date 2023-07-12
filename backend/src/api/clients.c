
#include "../server.h"
#include "../api.h"

#include "../plugin.h"
#include "../ts3remote.h"

static void mg_handler_get_clientlist_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/clients")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      struct TS3ClientList *list = &remote->clientList;

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_chunk(conn, "%s", "[");

      for (int i = 0; i < list->size; i++) {
        struct TS3Client *client = &list->items[i].client;
        struct TS3Channel *channel = &list->items[i].channel;

        mg_http_printf_json_chunk(conn, i ? ", %s" : "%s", "{" HTTP_JSON_CLIENT "," HTTP_JSON_CHANNEL "}",
          client->id, client->nickname, client->inputMuted, client->outputMuted, channel->id, channel->name);
      }

      mg_http_printf_chunk(conn, "]");
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_clientlist() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_clientlist_fn);
  return &handler;
}
