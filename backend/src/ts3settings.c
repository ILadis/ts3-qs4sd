
#include "ts3settings.h"
#include "sdinput.h"
#include "server.h"
#include "plugin.h"
#include "log.h"

const char* TS3Settings_getFilepath() {
  static char filepath[1024] = {0};

  if (filepath[0] != '\0') {
    return filepath;
  }

  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  if (ts3 == NULL) {
    return NULL;
  }

  char config[1024] = {0};
  ts3->getConfigPath(config, sizeof(config));

  int size = snprintf(filepath, sizeof(filepath), "%s%s", config, "plugins/ts3-qs4sd.json");
  if (size < sizeof(filepath)) {
    return filepath;
  }

  filepath[0] = '\0';
  return NULL;
}

void TS3Settings_load(struct TS3Remote *remote) {
  struct TS3Settings settings = {0};
  const char *config = TS3Settings_getFilepath();

  if (!TS3Settings_readFrom(&settings, config)) {
    return;
  }

  int id = 0;
  enum SDInputKey key;
  while (SDInputKey_byId(&key, id)) {
    const char *name = SDInputKey_getName(key);
    if (strncmp(settings.pttHotkey, name, sizeof(settings.pttHotkey)) == 0) {
      Logger_debugLog("Restored PTT hotkey from settings: %ld", key);
      TS3Remote_setPttHotkey(remote, id);
      break;
    }
    else id++;
  }
}

void TS3Settings_save(struct TS3Remote *remote) {
  struct TS3Settings settings = {0};
  const char *config = TS3Settings_getFilepath();

  enum SDInputKey key;
  if (SDInputKey_byId(&key, remote->pttHotkey)) {
    const char *name = SDInputKey_getName(key);
    snprintf(settings.pttHotkey, sizeof(settings.pttHotkey), "%s", name);
  }

  TS3Settings_writeTo(&settings, config);
}

bool TS3Settings_readFrom(struct TS3Settings *settings, const char *path) {
  FILE *file = fopen(path == NULL ? "" : path, "r");
  if (file == NULL) {
    Logger_errorLog("Could not open file to read settings: %s", path);
    return false;
  }

  char buffer[1024] = {0};
  int size = fread(buffer, sizeof(char), length(buffer), file);
  Logger_debugLog("Read settings: %s", buffer);

  struct mg_str json = mg_str_n(buffer, size);
  TS3Settings_fromJson(json, "$.ptt_hotkey", settings->pttHotkey);

  fclose(file);
  return true;
}

bool TS3Settings_writeTo(struct TS3Settings *settings, const char *path) {
  FILE *file = fopen(path == NULL ? "" : path, "w");
  if (file == NULL) {
    Logger_errorLog("Could not open file to write settings: %s", path);
    return false;
  }

  char buffer[1024] = {0};
  const char json[] = ""
    "{"
      "\"ptt_hotkey\":%m"
    "}";

  size_t size = mg_snprintf(buffer, sizeof(buffer), json, mg_json_string(settings->pttHotkey));
  fwrite(buffer, sizeof(char), size, file);
  Logger_debugLog("Wrote settings: %s", buffer);

  fclose(file);
  return true;
}
