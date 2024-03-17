#ifndef API_H
#define API_H

#include "server.h"
#include "settings.h"

// ts3 json objects
#define HTTP_JSON_SERVER    "\"name\":%m,\"status\":%d"
#define HTTP_JSON_BOOKMARK  "\"name\":%m,\"uuid\":%m"
#define HTTP_JSON_CLIENT    "\"client_id\":%lu,\"client_nickname\":%m,\"input_muted\":%s,\"output_muted\":%s"
#define HTTP_JSON_PTT_STATE "\"ptt_state\":%m,\"ptt_hotkey\":%m"
#define HTTP_JSON_CHANNEL   "\"channel_id\":%lu,\"channel_name\":%m,\"channel_order\":%lu,\"channel_max_clients\":%lu,\"channel_has_password\":%s"

// paudio json objects
#define HTTP_JSON_AUDIO_OUTPUT "\"index\":%d,\"name\":%m,\"volume\":%g,\"muted\":%m"

// common json objects
#define HTTP_JSON_EVENT "\"type\":%m"

#define USER_EVENT 100

enum EventType {
  CONNECTION_STATE_CONNECTED = USER_EVENT,
  CONNECTION_STATE_DISCONNECTED,
  BOOKMARKS_UPDATED,
  CLIENT_LIST_CHANGED,
  AUDIO_OUTPUTS_CHANGED,
  PTT_HOTKEYS_PRESSED,
};

struct mg_handler* mg_handler_get_server();
struct mg_handler* mg_handler_connect_server();
struct mg_handler* mg_handler_disconnect_server();

struct mg_handler* mg_handler_get_clientlist();
struct mg_handler* mg_handler_get_clientavatar();

struct mg_handler* mg_handler_get_self();
struct mg_handler* mg_handler_alter_ptt();
struct mg_handler* mg_handler_mute_toggle_self();
struct mg_handler* mg_handler_afk_toggle_self();

struct mg_handler* mg_handler_get_cursor();
struct mg_handler* mg_handler_move_cursor();
struct mg_handler* mg_handler_join_cursor();

struct mg_handler* mg_handler_get_browser();
struct mg_handler* mg_handler_move_browser();

struct mg_handler* mg_handler_events();

struct mg_handler* mg_handler_get_audio_outputs();
struct mg_handler* mg_handler_set_audio_output_volume();

static inline void mg_http_api_response(
    struct mg_connection *conn,
    const char *status,
    const char *media)
{
  mg_printf(conn, media == NULL
    ? "HTTP/1.1 %s\r\nContent-Length: 0\r\n"
    : "HTTP/1.1 %s\r\nTransfer-Encoding: chunked\r\nContent-Type: %s\r\n", status, media);
  mg_printf(conn, "Access-Control-Allow-Origin: " CORS_ORIGIN "\r\n\r\n");

  // signal end of response body
  conn->is_resp = 0;
}

#endif
