
#include "../server.h"
#include "../api.h"

#include "../paudio.h"

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
          output->index, output->name, output->volume, output->muted);
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

      if (!mg_http_get_json_integer(msg, "$.index", &index) || index < 0) {
        return mg_http_api_response(conn, "400 Bad Request", NULL);
      }

      double volume = 0.0;
      if (!mg_http_get_json_double(msg, "$.volume", &volume) || volume < 0) {
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
