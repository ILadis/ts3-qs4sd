#ifndef TS3REMOTE_H
#define TS3REMOTE_H

#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <fcntl.h>

#include "../vnd/ts3_functions.h"
#include "../vnd/teamspeak/public_rare_definitions.h"

/* A sample plugin can be found here:
 * https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/blob/master/src/plugin.c
 */

struct TS3Channel {
  uint64 id;
  char *name;
  int order;
  bool hasPassword;
  int maxClients;
};

struct TS3Bookmark {
  char *name;
  char *uuid;
};

struct TS3Remote {
  uint64 handle;

  struct TS3Server {
    int status;
    char *name;
    struct TS3Bookmark bookmarks[50];
    int numBookmarks;
  } server;

  struct TS3Client {
    anyID id;
    char *nickname;
    bool inputMuted;
    bool outputMuted;
  } client; // self

  int pttHotkey;

  // used to list all available clients and their current channel
  struct TS3ClientList {
    int size;
    struct {
      struct TS3Client client;
      struct TS3Channel channel;
    } items[50];
  } clientList;

  // used to peek into channel
  struct TS3Cursor {
    struct TS3Channel channel;
    struct TS3Client clients[50];
    int numClients;
  } cursor;

  // used to browse available channels
  struct TS3Browser {
    struct TS3Channel channel;
    struct TS3Channel childs[50];
    int numChilds;
  } browser;
};

#define TS3_PTT_HOTKEY_NONE   -1
#define TS3_PTT_HOTKEY_REBIND -2

#define TS3Remote_guardHandleIsset(remote, ...) do { if (remote->handle == 0) return __VA_ARGS__; } while(0);

struct TS3Remote* TS3Remote_getInstance(uint64 handle);

bool TS3Remote_loadBookmarks(struct TS3Remote *remote);
bool TS3Remote_connectBookmark(struct TS3Remote *remote, const char *uuid);

void TS3Remote_updateConnection(struct TS3Remote *remote);
bool TS3Remote_closeConnection(struct TS3Remote *remote);
void TS3Remote_resetConnection(struct TS3Remote *remote);

void TS3Remote_updateClient(struct TS3Remote *remote);
void TS3Remote_updateClientWithId(struct TS3Remote *remote, anyID clientId);
void TS3Remote_updateClientList(struct TS3Remote *remote);
int  TS3Remote_openClientAvatar(struct TS3Remote *remote, anyID clientId);

void TS3Remote_muteInput(struct TS3Remote *remote, bool mute);
void TS3Remote_muteOutput(struct TS3Remote *remote, bool mute);
void TS3Remote_setAfk(struct TS3Remote *remote, bool afk);
void TS3Remote_shouldTalk(struct TS3Remote *remote, bool talk);

void TS3Remote_setCursorToSelf(struct TS3Remote *remote);
void TS3Remote_setCursorToChannel(struct TS3Remote *remote, uint64 channelId);
void TS3Remote_updateCursor(struct TS3Remote *remote);
bool TS3Remote_joinCursor(struct TS3Remote *remote, const char *password);

void TS3Remote_setBrowserToSelf(struct TS3Remote *remote);
void TS3Remote_setBrowserToChannel(struct TS3Remote *remote, uint64 channelId);
void TS3Remote_updateBrowser(struct TS3Remote *remote);

#endif
