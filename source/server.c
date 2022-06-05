
#include "settings.h"

#include "server.h"
#include "injector.h"

#include <stdio.h>
#include <pthread.h>

static void mg_server_handler(struct mg_connection *conn, int event, void *data,void *context);
static void* mg_server_thread(void *context);

bool mg_server_start(
    struct mg_server *server,
    unsigned short port)
{
  server->running = true;
  server->port = port;

  struct mg_mgr *manager = &server->manager;
  mg_mgr_init(manager);

  char url[20];
  snprintf(url, sizeof(url), "0.0.0.0:%hu", port);

  struct mg_connection *conn = mg_http_listen(manager, url, mg_server_handler, server);
  if (conn == NULL) goto failure;

  pthread_t *thread = &server->thread;
  int result = pthread_create(thread, NULL, mg_server_thread, (void*) server);
  if (result != 0) goto failure;

  return true;

failure:
  mg_mgr_free(manager);

  server->running = false;
  server->port = 0;

  return false;
}

void mg_server_add_handler(
    struct mg_server *server,
    struct mg_handler *handler)
{
  handler->next = server->handler;
  server->handler = handler;
}

static void mg_server_handler(
    struct mg_connection *conn,
    int event, void *data, void *context)
{
  struct mg_server *server = context;
  struct mg_handler *handler = server->handler;

  while (handler != NULL) {
    handler->fn(conn, event, data);
    handler = handler->next;
  }
}

void mg_server_user_event(
    struct mg_server *server,
    int type, void *data)
{
  for (int i = 0; i < 10; i++) {
    struct mg_user_event *event = &server->events[i];

    if (event->type == 0) {
      event->data = data;
      event->type = type;
      return;
    }
  }
}

static struct mg_user_event* mg_server_next_user_event(struct mg_server *server) {
  for (int i = 0; i < 10; i++) {
    struct mg_user_event *event = &server->events[i];

    if (event->type != 0) {
      return event;
    }
  }

  return NULL;
}

static void mg_server_fire_event(
    struct mg_server *server,
    struct mg_user_event *event)
{
  struct mg_mgr *manager = &server->manager;
  struct mg_connection *conn = manager->conns, *next = NULL;

  while (conn != NULL) {
    next = conn->next;
    mg_call(conn, event->type, event->data);
    conn = next;
  }

  event->data = NULL;
  event->type = 0;
}

static void* mg_server_thread(void *context) {
  struct mg_server *server = context;
  struct mg_mgr *manager = &server->manager;
  struct mg_user_event *event = NULL;

  struct Injector *injector = Injector_createNew(manager);

  char injectUrl[60];
  snprintf(injectUrl, sizeof(injectUrl), "http://localhost:%hu/static/main.js", server->port);

  while (server->running) {
    mg_mgr_poll(manager, 100);

    event = mg_server_next_user_event(server);
    while (event != NULL) {
      mg_server_fire_event(server, event);
      event = mg_server_next_user_event(server);
    }

    Injector_tryLoadExternalJS(injector, injectUrl);
  }

  return NULL;
}

void mg_server_stop(struct mg_server *server) {
  server->running = false;

  pthread_t thread = server->thread;
  pthread_join(thread, NULL);

  struct mg_mgr *manager = &server->manager;
  mg_mgr_free(manager);

  server->handler = NULL;
  server->port = 0;
}

static void mg_handler_not_found_fn(
    struct mg_connection *conn,
    int event, void *data)
{
  if (event == MG_EV_HTTP_MSG) {
    struct mg_iobuf *send = &conn->send;
    if (send->len == 0) {
      mg_http_reply(conn, 404, NULL, "Requested resource not found!");
    }
  }
}

struct mg_handler* mg_handler_not_found() {
  static struct mg_handler handler = mg_handler_of(mg_handler_not_found_fn);
  return &handler;
}
