#ifndef DEVTOOLS_H
#define DEVTOOLS_H

#include "../vnd/mongoose.h"

struct DevTools {
  struct mg_mgr *manager;
  struct mg_connection *conn;
  char url[100], tab[255];
  char debuggerWsUrl[255];
};

#define DevTools_createNew(manager) &((struct DevTools){ manager })

void DevTools_inspectTab(
    struct DevTools *dev,
    const char *url, const char *tab);

bool DevTools_evaluateJSCode(
    struct DevTools *dev,
    const char *code);

#endif
