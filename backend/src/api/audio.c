
#include "../server.h"
#include "../api.h"

#include "../paudio.h"
#include "../ts3remote.h"

static void mg_handler_get_audio_outputs_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/audio/outputs")) {
      struct PAudio *paudio = PAudio_getInstance();
      struct PAudioOutput *output = NULL;

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_json_chunk(conn, "%s", "[");

      int i = 0;
      while (PAudio_nextOutput(paudio, &output)) {
        mg_http_printf_json_chunk(conn, i++ ? ", %s" : "%s", "{" HTTP_JSON_AUDIO_OUTPUT "}",
            mg_json_number(output->index), mg_json_string(output->name), mg_json_number(output->volume), mg_json_bool(output->muted));
      }

      mg_http_printf_chunk(conn, "]");
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_audio_outputs() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_audio_outputs_fn);
  return &handler;
}

static void mg_handler_set_audio_output_volume_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/audio/outputs/volume")) {
      int index = 0;

      if (!mg_json_get_integer(msg->body, "$.index", &index) || index < 0) {
        return mg_http_api_response(conn, "400 Bad Request", NULL);
      }

      double volume = 0.0;
      if (!mg_json_get_double(msg->body, "$.volume", &volume) || volume < 0) {
        return mg_http_api_response(conn, "400 Bad Request", NULL);
      }

      struct PAudio *paudio = PAudio_getInstance();
      bool result = PAudio_setOutputVolume(paudio, index, volume);

      return mg_http_api_response(conn, result ? "200 OK" : "500 Internal Server Error", NULL);
    }
  }
}

struct mg_handler* mg_handler_set_audio_output_volume() {
  static struct mg_handler handler = mg_handler_of(mg_handler_set_audio_output_volume_fn);
  return &handler;
}

static void mg_handler_get_audio_inputs_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/api/audio/inputs")) {
      struct TS3Remote *remote = TS3Remote_getInstance(0);
      struct TS3Server *server = &remote->server;
      struct TS3CaptureDevice *devices = &server->captureDevices[0];

      TS3Remote_updateCaptureDevices(remote);

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_json_chunk(conn, "%s", "[");

      for (int i = 0; i < server->numCaptureDevices; i++) {
        mg_http_printf_json_chunk(conn, i ? ", %s" : "%s", "{" HTTP_JSON_AUDIO_INPUT "}",
            mg_json_string(devices[i].id), mg_json_string(devices[i].name), mg_json_bool(devices[i].isCurrent));
      }

      mg_http_printf_chunk(conn, "]");
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_audio_inputs() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_audio_inputs_fn);
  return &handler;
}

static void mg_handler_set_current_audio_input_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;

    if (mg_http_reqmatch(msg, HTTP_METHOD_POST, "/api/audio/inputs/current")) {
      char id[256];

      if (!mg_json_get_string(msg->body, "$.id", id, sizeof(id))) {
        return mg_http_api_response(conn, "400 Bad Request", NULL);
      }

      struct TS3Remote *remote = TS3Remote_getInstance(0);
      bool result = TS3Remote_openCaptureDevice(remote, id);

      return mg_http_api_response(conn, result ? "200 OK" : "500 Internal Server Error", NULL);
    }
  }
}

struct mg_handler* mg_handler_set_current_audio_input() {
  static struct mg_handler handler = mg_handler_of(mg_handler_set_current_audio_input_fn);
  return &handler;
}
