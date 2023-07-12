
#include "../server.h"
#include "../api.h"

#include "../plugin.h"
#include "../ts3remote.h"

static void mg_handler_get_server_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/server")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      struct TS3Server *server = &remote->server;
      struct TS3Bookmark *bookmarks = &server->bookmarks[0];

      bool result = TS3Remote_loadBookmarks(remote);
      if (!result) {
        return mg_http_api_response(conn, "500 Internal Server Error", NULL);
      }

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_json_chunk(conn, "%s", "{" HTTP_JSON_SERVER ",\"bookmarks\":[", server->name, server->status);
  
      for (int i = 0; i < server->numBookmarks; i++) {
        mg_http_printf_json_chunk(conn, i ? ", %s" : "%s", "{" HTTP_JSON_BOOKMARK "}", bookmarks[i].name, bookmarks[i].uuid);
      }

      mg_http_printf_chunk(conn, "]}");
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_server() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_server_fn);
  return &handler;
}

static void mg_handler_connect_server_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/server/connect")) {
      char uuid[40];

      if (!mg_http_get_json_string(msg, "$.uuid", uuid, sizeof(uuid))) {
        return mg_http_api_response(conn, "400 Bad Request", NULL);
      }

      struct TS3Remote *remote = TS3Remote_getInstance(0);
      bool result = TS3Remote_connectBookmark(remote, uuid);

      return mg_http_api_response(conn, result ? "200 OK" : "500 Internal Server Error", NULL);
    }
  }
}

struct mg_handler* mg_handler_connect_server() {
  static struct mg_handler handler = mg_handler_of(mg_handler_connect_server_fn);
  return &handler;
}

static void mg_handler_disconnect_server_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/server/disconnect")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      bool result = TS3Remote_closeConnection(remote);

      return mg_http_api_response(conn, result ? "200 OK" : "500 Internal Server Error", NULL);
    }
  }
}

struct mg_handler* mg_handler_disconnect_server() {
  static struct mg_handler handler = mg_handler_of(mg_handler_disconnect_server_fn);
  return &handler;
}
