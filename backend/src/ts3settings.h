#ifndef TS3SETTINGS_H
#define TS3SETTINGS_H

#include <stdbool.h>
#include <stdio.h>

#include <string.h>

#include "ts3remote.h"

struct TS3Settings {
  char pttHotkey[3];
};

const char* TS3Settings_getFilepath();

void TS3Settings_load(struct TS3Remote *remote);
void TS3Settings_save(struct TS3Remote *remote);

bool TS3Settings_readFrom(struct TS3Settings *settings, const char *path);
bool TS3Settings_writeTo(struct TS3Settings *settings, const char *path);

#define TS3Settings_fromJson(json, path, key) do { \
  memset(key, '\0', sizeof(key)); \
  mg_json_get_string(json, path, key, sizeof(key)); \
} while (0);


#endif
