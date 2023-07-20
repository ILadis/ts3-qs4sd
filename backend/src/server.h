#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "../vnd/mongoose.h"
#include "../vnd/mjson.h"

struct mg_server {
  volatile bool running;
  unsigned short port;
  struct mg_mgr manager;

  struct mg_handler {
    void (*fn)(struct mg_connection *conn, int event, void *data);
    struct mg_handler *next;
  } *handler;

  struct mg_user_event {
    volatile int type;
    void *data;
  } events[10];

  pthread_t thread;
};

#define mg_handler_of(fn) (struct mg_handler){ fn, NULL }

bool mg_server_start(
    struct mg_server *server,
    unsigned short port);

void mg_server_add_handler(
    struct mg_server *server,
    struct mg_handler *handler);

void mg_server_user_event(
    struct mg_server *server,
    int type, void *data);

void mg_server_stop(struct mg_server *server);

struct mg_handler* mg_handler_not_found();

#define HTTP_RESPONSE(status, media) "HTTP/1.1 " status "\r\nTransfer-Encoding: chunked\r\nContent-Type: " media "\r\n"
#define HTTP_REQUEST(host, method, path) method " " path " HTTP/1.1\r\nHost: " host "\r\n"
#define HTTP_HEADER(name, value) name ": " value "\r\n"
#define HTTP_NEWLINE "\r\n"

#define HTTP_METHOD_GET  mg_str_n("GET", 3)
#define HTTP_METHOD_POST mg_str_n("POST", 4)

static inline bool mg_http_reqmatch(
    struct mg_http_message *msg,
    const struct mg_str method,
    const char *uri)
{
  return mg_strcmp(msg->method, method) == 0 && mg_http_match_uri(msg, uri);
}

static inline int mg_http_reqmatchfmt(
    struct mg_http_message *msg,
    const struct mg_str method,
    const char *uri, ...)
{
  if (mg_strcmp(msg->method, method) != 0) {
    return 0;
  }

  va_list arguments;
  va_start(arguments, uri);
  int result = vsscanf(msg->uri.ptr, uri, arguments);
  va_end(arguments);

  return result;
}

static inline void mg_http_printf_json_chunk(
    struct mg_connection *conn,
    const char *fmt,
    const char *json, ...)
{
  static char buf[250];
  struct mjson_fixedbuf fb = { buf, sizeof(buf), 0 };

  va_list arguments;
  va_start(arguments, json);
  mjson_vprintf(mjson_print_fixed_buf, &fb, json, &arguments);
  va_end(arguments);

  mg_http_printf_chunk(conn, fmt, buf);
}

static inline bool mg_http_get_json_string(
    struct mg_http_message *msg,
    const char *path,
    char *value, int len)
{
  return mjson_get_string(msg->body.ptr, msg->body.len, path, value, len) != -1;
}

static inline bool mg_ws_get_json_string(
    struct mg_ws_message *ws,
    const char *path,
    char *value, int len)
{
  return mjson_get_string(ws->data.ptr, ws->data.len, path, value, len) != -1;
}

static inline bool mg_http_get_json_integer(
    struct mg_http_message *msg,
    const char *path,
    int *value)
{
  double target;
  if (mjson_get_number(msg->body.ptr, msg->body.len, path, &target) != 0) {
    *value = (int) target;
    return true;
  }

  return false;
}

static inline bool mg_http_get_json_double(
    struct mg_http_message *msg,
    const char *path,
    double *value)
{
  double target;
  if (mjson_get_number(msg->body.ptr, msg->body.len, path, &target) != 0) {
    *value = target;
    return true;
  }

  return false;
}

static inline void mg_http_printf_sse_json_chunk(
    struct mg_connection *conn,
    const char *json, ...)
{
  static char buf[250];
  struct mjson_fixedbuf fb = { buf, sizeof(buf), 0 };

  va_list arguments;
  va_start(arguments, json);
  mjson_vprintf(mjson_print_fixed_buf, &fb, json, &arguments);
  va_end(arguments);

  mg_http_printf_chunk(conn, "data: %s\r\n\r\n", buf);
}

#endif
