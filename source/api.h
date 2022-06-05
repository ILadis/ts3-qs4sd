#ifndef API_H
#define API_H

#include "server.h"
#include "settings.h"

#define HTTP_JSON_SERVER   "\"name\":%Q,\"status\":%d"
#define HTTP_JSON_BOOKMARK "\"name\":%Q,\"uuid\":%Q"
#define HTTP_JSON_CLIENT   "\"client_id\":%lu,\"client_nickname\":%Q,\"input_muted\":%B,\"output_muted\":%B"
#define HTTP_JSON_CHANNEL  "\"channel_id\":%lu,\"channel_name\":%Q"
#define HTTP_JSON_EVENT    "\"type\":%Q"

struct mg_handler* mg_handler_get_server();
struct mg_handler* mg_handler_connect_server();
struct mg_handler* mg_handler_disconnect_server();

struct mg_handler* mg_handler_get_clientlist();

struct mg_handler* mg_handler_get_self();
struct mg_handler* mg_handler_mute_toggle_self();
struct mg_handler* mg_handler_afk_toggle_self();

struct mg_handler* mg_handler_get_cursor();
struct mg_handler* mg_handler_move_cursor();
struct mg_handler* mg_handler_join_cursor();

struct mg_handler* mg_handler_get_browser();
struct mg_handler* mg_handler_move_browser();

struct mg_handler* mg_handler_events();

struct mg_handler* mg_handler_static_resources();

static inline void mg_http_api_response(
    struct mg_connection *conn,
    const char *status,
    const char *media)
{
  mg_printf(conn, media == NULL
    ? "HTTP/1.1 %s\r\nContent-Length: 0\r\n"
    : "HTTP/1.1 %s\r\nTransfer-Encoding: chunked\r\nContent-Type: %s\r\n", status, media);
  mg_printf(conn, "Access-Control-Allow-Origin: " CORS_ORIGIN "\r\n\r\n");
}

#endif
