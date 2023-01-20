
#include "../server.h"
#include "../api.h"

#include "resource.h"

INCBIN(test_html, "source/static/test.html");
INCBIN(main_js, "source/static/main.js");
INCBIN(modules_js, "source/static/modules.js");
INCBIN(components_js, "source/static/components.js");
INCBIN(views_js, "source/static/views.js");
INCBIN(utils_js, "source/static/utils.js");
INCBIN(client_js, "source/static/client.js");
INCBIN(styles_css, "source/static/styles.css");
INCBIN(logo_svg, "source/static/logo.svg");

static struct mg_resource resources[] = {
  mg_resource_of("/static/test.html", "text/html", test_html),
  mg_resource_of("/static/main.js", "text/javascript", main_js),
  mg_resource_of("/static/modules.js", "text/javascript", modules_js),
  mg_resource_of("/static/components.js", "text/javascript", components_js),
  mg_resource_of("/static/views.js", "text/javascript", views_js),
  mg_resource_of("/static/utils.js", "text/javascript", utils_js),
  mg_resource_of("/static/client.js", "text/javascript", client_js),
  mg_resource_of("/static/styles.css", "text/css", styles_css),
  mg_resource_of("/static/logo.svg", "image/svg+xml", logo_svg),
  mg_resource_empty()
};

static void mg_handler_static_resources_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_http_message *msg = (struct mg_http_message *) data;
    struct mg_resource *resource = &resources[0];
    bool found = false;

    if (mg_http_reqmatch(msg, HTTP_METHOD_GET, "/static/**")) {
      while (resource->path != NULL) {
        if (strncmp(resource->path, msg->uri.ptr, msg->uri.len) == 0) {
          found = true;
          break;
        }

        resource++;
      }

      mg_printf(conn, "HTTP/1.1 %s\r\n", found ? "200 OK" : "404 Not Found");
      mg_printf(conn, "Content-Length: %d\r\n", *resource->size);
      mg_printf(conn, "Content-Type: %s\r\n", resource->media);
      mg_printf(conn, "Access-Control-Allow-Origin: " CORS_ORIGIN "\r\n\r\n");
      mg_printf(conn, "%.*s", *resource->size, resource->data);

      // signal end of response body
      conn->is_resp = 0;
    }
  }
}

struct mg_handler* mg_handler_static_resources() {
  static struct mg_handler handler = mg_handler_of(mg_handler_static_resources_fn);
  return &handler;
}
