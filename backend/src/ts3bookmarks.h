#ifndef TS3BOOKMAKRS_H
#define TS3BOOKMAKRS_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include <time.h>
#include <string.h>
#include <uuid/uuid.h>

#include <sqlite3.h>

struct TS3BookmarkEntry {
  unsigned long timestamp;
  struct {
    char self[UUID_STR_LEN];
    char parent[UUID_STR_LEN];
    char folder[UUID_STR_LEN];
  } uuids;
  char bookmarkName[512];
  char serverAddress[512];
  char serverPassword[512];
  unsigned int serverPort;
  char nickname[128];
};

struct TS3BookmarkManager {
  sqlite3 *db;
};

bool
TS3BookmarkManager_addBookmark(
    const char *dbpath,
    const char *nickname,
    const char *bookmarkName,
    const char *serverAddress,
    unsigned int serverPort);

struct TS3BookmarkManager* TS3BookmarkManager_getInstance();
bool TS3BookmarkManager_openDatabase(struct TS3BookmarkManager *manager, const char *dbpath);
void TS3BookmarkManager_closeDatabase(struct TS3BookmarkManager *manager);
bool TS3BookmarkManager_appendNewBookmark(struct TS3BookmarkManager *manager, struct TS3BookmarkEntry *entry);

bool TS3Bookmarks_deserialize(struct TS3BookmarkEntry *entry, const unsigned char *buffer, int size);
int TS3Bookmarks_serialize(struct TS3BookmarkEntry *entry, unsigned char *buffer, int size);

#endif
