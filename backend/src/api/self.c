
#include "../server.h"
#include "../api.h"

#include "../ts3remote.h"
#include "../sdinput.h"

static void mg_handler_get_self_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/self")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      struct TS3Client *client = &remote->client;

      struct SDInput *input = SDInput_getInstance();
      const char *pttHotkeyName = "";
      const char *pttState = "unavailable";

      if (input->fd > 0) {
        switch (remote->pttHotkey) {
        case TS3_PTT_HOTKEY_NONE:
          pttState = "disabled";
          break;
        case TS3_PTT_HOTKEY_REBIND:
          pttState = "rebinding";
          break;
        }

        enum SDInputKey pttHotkey;
        if (SDInputKey_byId(&pttHotkey, remote->pttHotkey)) {
          pttHotkeyName = SDInputKey_getName(pttHotkey);
          pttState = "active";
        }
      }

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_json_chunk(conn, "%s", "{" HTTP_JSON_CLIENT "," HTTP_JSON_PTT_STATE "}",
          client->id, client->nickname, client->inputMuted, client->outputMuted, pttState, pttHotkeyName);
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_self() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_self_fn);
  return &handler;
}

static void mg_handler_alter_ptt_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/self/ptt/rebind")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      remote->pttHotkey = TS3_PTT_HOTKEY_REBIND;
      TS3Remote_shouldTalk(remote, true);
      return mg_http_api_response(conn, "200 OK", NULL);
    }

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/self/ptt/clear")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      remote->pttHotkey = TS3_PTT_HOTKEY_NONE;
      TS3Remote_shouldTalk(remote, true);
      return mg_http_api_response(conn, "200 OK", NULL);
    }
  }
}

struct mg_handler* mg_handler_alter_ptt() {
  static struct mg_handler handler = mg_handler_of(mg_handler_alter_ptt_fn);
  return &handler;
}

static inline void mg_handle_mute_self_request(
    struct mg_connection *conn,
    struct mg_http_message *msg,
    bool mute)
{
  char device[10];

  if (!mg_http_get_json_string(msg, "$.device", device, sizeof(device))) {
    return mg_http_api_response(conn, "400 Bad Request", NULL);
  }

  struct TS3Remote *remote = TS3Remote_getInstance(0);

  if (strcmp("input", device) == 0) {
    TS3Remote_muteInput(remote, mute);
    return mg_http_api_response(conn, "200 OK", NULL);
  }

  if (strcmp("output", device) == 0) {
    TS3Remote_muteOutput(remote, mute);
    return mg_http_api_response(conn, "200 OK", NULL);
  }

  return mg_http_api_response(conn, "400 Bad Request", NULL);
}

static void mg_handler_mute_toggle_self_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/self/mute")) {
      return mg_handle_mute_self_request(conn, msg, true);
    }

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/self/unmute")) {
      return mg_handle_mute_self_request(conn, msg, false);
    }
  }
}

struct mg_handler* mg_handler_mute_toggle_self() {
  static struct mg_handler handler = mg_handler_of(mg_handler_mute_toggle_self_fn);
  return &handler;
}

static inline void mg_handle_afk_self_request(
    struct mg_connection *conn,
    struct mg_http_message *msg,
    bool afk)
{
  struct TS3Remote *remote = TS3Remote_getInstance(0);
  TS3Remote_setAfk(remote, afk);

  return mg_http_api_response(conn, "200 OK", NULL);
}

static void mg_handler_afk_toggle_self_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/self/afk")) {
      return mg_handle_afk_self_request(conn, msg, true);
    }

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/self/unafk")) {
      return mg_handle_afk_self_request(conn, msg, false);
    }
  }
}

struct mg_handler* mg_handler_afk_toggle_self() {
  static struct mg_handler handler = mg_handler_of(mg_handler_afk_toggle_self_fn);
  return &handler;
}
