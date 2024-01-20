#ifndef TS3BOOKMAKRS_H
#define TS3BOOKMAKRS_H

#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <sqlite3.h>

struct TS3BookmarkEntry {
  char guid[37], puid[37];
  unsigned long timestamp;
  char bookmarkName[512];
  char serverAddress[512];
  char serverPassword[512];
  unsigned int serverPort;
  char nickname[128];
};

struct TS3BookmarkManager {
  sqlite3 *db;
};

struct TS3BookmarkManager* TS3BookmarkManager_getInstance();
bool TS3BookmarkManager_openDatabase(struct TS3BookmarkManager *manager, const char *dbpath);
bool TS3BookmarkManager_lastBookmarkUUID(struct TS3BookmarkManager *manager, char *uuid);

bool TS3Bookmarks_deserialize(struct TS3BookmarkEntry *entry, const unsigned char *buffer, int size);
int TS3Bookmarks_serialize(struct TS3BookmarkEntry *entry, unsigned char *buffer, int size);

#endif
