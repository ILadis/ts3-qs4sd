#ifndef INJECTOR_H
#define INJECTOR_H

#include "server.h"
#include "log.h"

struct Injector {
  struct mg_mgr *manager;

  enum {
    STATE_FETCH_WS_URL = 1,
    STATE_EVALUATE_JS_CODE,
    STATE_RETRY,
  } state;

  char wsUrl[250];
  char jsCode[500];
};

#define Injector_createNew(manager) &(struct Injector){ manager }

static inline void Injector_setState(
    struct Injector *injector,
    int state)
{
  injector->state = state;
}

static inline void Injector_tryLoadExternalJS(
    struct Injector *injector,
    const char *url)
{
  void Injector_gotoFetchWSUrlState(struct Injector *injector);

  static const char jsCode[] = "(function() {"
  "  for (let s of document.scripts) {"
  "    if (s.src == '%s') {"
  "      return;"
  "    }"
  "  }"
  "  let s = document.createElement('script');"
  "  s.src = '%s';"
  "  s.async = true;"
  "  s.type = 'module';"
  "  document.head.appendChild(s);"
  "})();";

  if (injector->state == 0) {
    snprintf(injector->jsCode, sizeof(injector->jsCode), jsCode, url, url);
    Injector_gotoFetchWSUrlState(injector);
  }
}

#endif
