#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "../vnd/mongoose.h"

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

#define mg_json_number(val) val
#define mg_json_string(val) mg_print_esc, 0, val
#define mg_json_bool(val)   val ? "true" : "false"

static inline void mg_http_printf_json_chunk(
    struct mg_connection *conn,
    const char *fmt,
    const char *json, ...)
{
  static char buf[1024];

  va_list arguments;
  va_start(arguments, json);
  mg_vsnprintf(buf, sizeof(buf), json, &arguments);
  va_end(arguments);

  mg_http_printf_chunk(conn, fmt, buf);
}

static inline void mg_http_printf_sse_json_chunk(
    struct mg_connection *conn,
    const char *json, ...)
{
  static char buf[250];

  va_list arguments;
  va_start(arguments, json);
  mg_vsnprintf(buf, sizeof(buf), json, &arguments);
  va_end(arguments);

  mg_http_printf_chunk(conn, "data: %s\r\n\r\n", buf);
}

static inline bool mg_json_find(
    struct mg_str json,
    const char *path,
    struct mg_str *value)
{
  int len, offset = mg_json_get(json, path, &len);
  if (offset < 0) {
    return false;
  }

  value->ptr = &json.ptr[offset];
  value->len = len;
  return true;
}

static inline bool mg_json_get_string(
    struct mg_str json,
    const char *path,
    char *value, int len)
{
  int size, offset = mg_json_get(json, path, &size);
  if (offset < 0) {
    return false;
  }

  // check if this is actually a json string value
  if (json.ptr[offset] != '"' || json.ptr[offset + size - 1] != '"') {
    return false;
  }

  offset += 1;
  size -= 2;

  // need one extra byte for '\0'
  if (size + 1 > len) {
    return false;
  }

  for (int i = 0; i < size; i++) {
    value[i] = json.ptr[offset + i];
  }

  value[size] = '\0';
  return true;
}

static inline bool mg_json_get_integer(
    struct mg_str json,
    const char *path,
    int *value)
{
  double target;
  if (mg_json_get_num(json, path, &target) != 0) {
    *value = (int) target;
    return true;
  }

  return false;
}

static inline bool mg_json_get_double(
    struct mg_str json,
    const char *path,
    double *value)
{
  double target;
  if (mg_json_get_num(json, path, &target) != 0) {
    *value = target;
    return true;
  }

  return false;
}

#endif
