#ifndef PLUGIN_H
#define PLUGIN_H

#include <stddef.h>
#include <stdarg.h>

#include "../vnd/ts3_functions.h>

struct TS3Functions* ts3plugin_getFunctionPointers();
struct mg_server* ts3plugin_getServer();

#endif
