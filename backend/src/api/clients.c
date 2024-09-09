
#include "../server.h"
#include "../api.h"

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
            mg_json_number(client->id), mg_json_string(client->nickname), mg_json_bool(client->inputMuted),
            mg_json_bool(client->outputMuted), mg_json_number(channel->id), mg_json_string(channel->name),
            mg_json_number(channel->order), mg_json_bool(channel->hasChannels), mg_json_bool(channel->hasPassword));
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

static void mg_handler_get_clientavatar_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;
    int clientId = 0;

    if (mg_http_reqmatchfmt(msg, HTTP_METHOD_GET, "/api/clients/%d/avatar", &clientId) == 1) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      int fd = TS3Remote_openClientAvatar(remote, clientId);

      if (fd == 0) {
        return mg_http_api_response(conn, "202 Accepted", NULL);
      }

      if (fd < 0) {
        return mg_http_api_response(conn, "404 Not Found", NULL);
      }

      mg_http_api_response(conn, "200 OK", "application/octet-stream");

      char buffer[512];
      unsigned int size;

      do {
        size = read(fd, buffer, sizeof(buffer));
        mg_http_write_chunk(conn, buffer, size);
      } while (size > 0);

      close(fd);
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_clientavatar() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_clientavatar_fn);
  return &handler;
}
