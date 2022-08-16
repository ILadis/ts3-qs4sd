
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
      PAudio_updateOutputs(paudio);

      struct PAudioOutput *output = &paudio->outputs[0];

      mg_http_api_response(conn, "200 OK", "application/json");
      mg_http_printf_json_chunk(conn, "%s", "{\"index\":%d,\"name\":%Q,\"volume\":%g,\"muted\":%B}", output->index, output->name, output->volume, output->muted);
      mg_http_printf_chunk(conn, "");
    }
  }
}

struct mg_handler* mg_handler_get_audio_outputs() {
  static struct mg_handler handler = mg_handler_of(mg_handler_get_audio_outputs_fn);
  return &handler;
}
