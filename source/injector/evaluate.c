
#include "states.h"

static void Injector_evaluateJSCodeFn(
    struct mg_connection *conn,
    int event, void *data, void *context)
{
  if (event == MG_EV_WS_OPEN) {
    struct Injector *injector = (struct Injector *) context;

    char message[1024];
    const char json[] = "{"
        "\"id\":%d,"
        "\"method\":\"Runtime.evaluate\","
        "\"params\":{"
            "\"expression\":%Q,"
            "\"userGesture\":true,"
            "\"awaitPromise\":true"
        "}}";

    int length = mjson_snprintf(message, sizeof(message), json, 1, injector->jsCode);
    mg_ws_send(conn, message, length, WEBSOCKET_OP_TEXT);
  }
  else if (event == MG_EV_WS_MSG) {
    struct Injector *injector = (struct Injector *) context;
    conn->is_closing = 1;
    return Injector_gotoRetryState(injector);
  }
}

void Injector_gotoEvaluateJSCodeState(struct Injector *injector) {
  injector->state = STATE_EVALUATE_JS_CODE;
  injector->conn = mg_ws_connect(injector->manager, injector->wsUrl, Injector_evaluateJSCodeFn, injector, NULL);
}