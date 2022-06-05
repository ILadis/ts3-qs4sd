#ifndef PLUGIN_H
#define PLUGIN_H

#include <stddef.h>
#include <vendor/ts3_functions.h>

struct TS3Functions* ts3plugin_getFunctionPointers();
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs);

#endif
