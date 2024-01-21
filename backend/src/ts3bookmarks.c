
#include "ts3bookmarks.h"
#include "protobuf.h"
#include "log.h"

bool
TS3BookmarkManager_addBookmark(
    const char *dbpath,
    const char *nickname,
    const char *bookmarkName,
    const char *serverAddress,
    unsigned int serverPort)
{
  struct TS3BookmarkManager manager = {0};
  if (!TS3BookmarkManager_openDatabase(&manager, dbpath)) {
    Logger_errorLog("Could not open database: %s", dbpath);
    return false;
  }

  struct TS3BookmarkEntry entry = { .serverPort = serverPort };

  snprintf(entry.nickname, sizeof(entry.nickname), "%s", nickname);
  snprintf(entry.bookmarkName, sizeof(entry.bookmarkName), "%s", bookmarkName);
  snprintf(entry.serverAddress, sizeof(entry.serverAddress), "%s", serverAddress);

  bool result = TS3BookmarkManager_appendNewBookmark(&manager, &entry);
  TS3BookmarkManager_closeDatabase(&manager);

  return result;
}

struct TS3BookmarkManager* TS3BookmarkManager_getInstance() {
  static struct TS3BookmarkManager manager = {0};
  return &manager;
}

bool TS3BookmarkManager_openDatabase(
    struct TS3BookmarkManager *manager,
    const char *dbpath)
{
  if (manager->db != NULL) {
    return true;
  }

  int result = sqlite3_open(dbpath, &manager->db);
  if (result != SQLITE_OK) {
    TS3BookmarkManager_closeDatabase(manager);
    return false;
  }

  return true;
}

void TS3BookmarkManager_closeDatabase(struct TS3BookmarkManager *manager) {
  sqlite3_close(manager->db);
  manager->db = NULL;
}

static bool TS3BookmarkManager_findBookmarkByUUIDs(
    struct TS3BookmarkManager *manager,
    struct TS3BookmarkEntry *entry,
    const char *folder, const char *parent)
{
  sqlite3_stmt *stmt = NULL;
  int result = sqlite3_prepare(manager->db, "SELECT \"value\" FROM ProtobufItems", -1, &stmt, NULL);

  if (result != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *buffer = sqlite3_column_blob(stmt, 0);
    int size = sqlite3_column_bytes(stmt, 0);

    struct TS3BookmarkEntry next = {0};
    bool result = TS3Bookmarks_deserialize(&next, buffer, size);

    if (result) {
      int foldercmp = strcmp(next.uuids.folder, folder);
      int parentcmp = strcmp(next.uuids.parent, parent);

      if (foldercmp == 0 && parentcmp == 0) {
        memcpy(entry, &next, sizeof(next));
        sqlite3_finalize(stmt);
        return true;
      }
    }
  }

  sqlite3_finalize(stmt);
  return false;
}

static void TS3BookmarkManager_setSelfUUID(
    struct TS3BookmarkManager *manager,
    struct TS3BookmarkEntry *entry)
{
  uuid_t uuid;
  uuid_generate_random(uuid);
  uuid_unparse(uuid, entry->uuids.self);
  time((time_t *) &entry->timestamp);
}

static void TS3BookmarkManager_setParentUUID(
    struct TS3BookmarkManager *manager,
    struct TS3BookmarkEntry *entry)
{
  struct TS3BookmarkEntry next = {0};
  const char *folder = "";

  bool result = TS3BookmarkManager_findBookmarkByUUIDs(manager, &next, folder, "");
  if (result) {
    while (TS3BookmarkManager_findBookmarkByUUIDs(manager, &next, folder, next.uuids.self));
    sprintf(entry->uuids.parent, "%s", next.uuids.self);
  }
}

static bool TS3BookmarkManager_nextBookmarkKey(
    struct TS3BookmarkManager *manager,
    char key[16])
{
  sqlite3_stmt *stmt = NULL;
  int result = sqlite3_prepare(manager->db, "SELECT \"key\" FROM ProtobufItems", -1, &stmt, NULL);

  if (result != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }

  int next = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *buffer = sqlite3_column_text(stmt, 0);

    int current = atoi((const char *) buffer);
    if (current > next) {
      next = current;
    }
  }

  snprintf(key, 16, "%d", next + 1);
  sqlite3_finalize(stmt);

  return true;
}

static bool TS3BookmarkManager_insertBookmark(
    struct TS3BookmarkManager *manager,
    struct TS3BookmarkEntry *entry,
    char *key)
{

  unsigned char buffer[2048] = {0};
  int length = TS3Bookmarks_serialize(entry, buffer, sizeof(buffer));

  sqlite3_stmt *stmt = NULL;
  int result = sqlite3_prepare(manager->db, "INSERT INTO ProtobufItems (\"timestamp\", \"key\", \"value\") VALUES (?, ?, ?)", -1, &stmt, NULL);

  if (result != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_bind_int (stmt, 1, entry->timestamp);
  sqlite3_bind_text(stmt, 2, key, strlen(key), SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, buffer, length, SQLITE_STATIC);

  bool success = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);

  return success;
}

static bool TS3BookmarkManager_calculateChecksum(
    struct TS3BookmarkManager *manager,
    unsigned char checksum[20])
{
  sqlite3_stmt *stmt = NULL;
  int result = sqlite3_prepare(manager->db, "SELECT \"value\" FROM ProtobufItems WHERE \"key\"!='Checksum'", -1, &stmt, NULL);

  if (result != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }

  SHA1_CTX sha1 = {0};
  SHA1Init(&sha1);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *buffer = sqlite3_column_blob(stmt, 0);
    int size = sqlite3_column_bytes(stmt, 0);

    if (size > 0) {
      SHA1Update(&sha1, buffer, size);
    }
  }

  SHA1Final(checksum, &sha1);
  sqlite3_finalize(stmt);

  return true;
}

static bool TS3BookmarkManager_updateChecksum(
    struct TS3BookmarkManager *manager,
    unsigned char checksum[20])
{
  sqlite3_stmt *stmt = NULL;
  int result = sqlite3_prepare(manager->db, "UPDATE ProtobufItems SET \"value\"=? WHERE \"key\"='Checksum'", -1, &stmt, NULL);

  if (result != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_blob(stmt, 1, checksum, 20, SQLITE_STATIC);

  bool success = sqlite3_step(stmt) == SQLITE_DONE;
  sqlite3_finalize(stmt);

  return success;
}

bool TS3BookmarkManager_appendNewBookmark(
    struct TS3BookmarkManager *manager,
    struct TS3BookmarkEntry *entry)
{
  char key[16] = {0};
  if (!TS3BookmarkManager_nextBookmarkKey(manager, key)) {
    Logger_errorLog("Failed to append new bookmark: could not find next bookmark key");
    return false;
  }

  Logger_debugLog("Using '%s' as next key for new bookmark entry", key);

  TS3BookmarkManager_setSelfUUID(manager, entry);
  Logger_debugLog("Assigned %s as self UUID for new bookmark entry", entry->uuids.self);

  TS3BookmarkManager_setParentUUID(manager, entry);
  Logger_debugLog("Assigned %s as parent UUID for new bookmark entry", entry->uuids.parent);

  if (!TS3BookmarkManager_insertBookmark(manager, entry, key)) {
    Logger_errorLog("Failed to append new bookmark: could not insert new bookmark entry");
    return false;
  }

  unsigned char checksum[20] = {0};
  if (!TS3BookmarkManager_calculateChecksum(manager, checksum)) {
    Logger_errorLog("Failed to append new bookmark: could not calculate new checksum");
    return false;
  }

  if (!TS3BookmarkManager_updateChecksum(manager, checksum)) {
    Logger_errorLog("Failed to append new bookmark: could not update new checksum");
    return false;
  }

  return true;
}

static bool TS3BookmarkEntry_deserialize(
    struct TS3BookmarkEntry *entry,
    unsigned char *buffer, int size)
{
  pb_istream_t stream = pb_istream_from_buffer(buffer, size);

  pb_istream_read_string(&stream, entry->bookmarkName, sizeof(entry->bookmarkName), 1);
  pb_istream_read_string(&stream, entry->serverAddress, sizeof(entry->serverAddress), 2);
  pb_istream_read_varint(&stream, &entry->serverPort, 3);
  pb_istream_read_string(&stream, entry->nickname, sizeof(entry->nickname), 4);
  pb_istream_read_buffer(&stream, NULL, 0, 5);
  pb_istream_read_buffer(&stream, NULL, 0, 6);
  pb_istream_read_string(&stream, entry->serverPassword, sizeof(entry->serverPassword), 7);

  return true;
}

bool TS3Bookmarks_deserialize(
    struct TS3BookmarkEntry *entry,
    const unsigned char *buffer, int size)
{
  pb_istream_t stream = pb_istream_from_buffer(buffer, size);

  unsigned int type = -1;
  unsigned int subtype = -1;

  pb_istream_read_string(&stream, entry->uuids.self, sizeof(entry->uuids.self), 2);
  pb_istream_read_varint(&stream, &type, 4);
  pb_istream_read_buffer(&stream, NULL, 0, 5);
  pb_istream_read_varint(&stream, &subtype, 6);
  pb_istream_read_string(&stream, entry->uuids.folder, sizeof(entry->uuids.folder), 7);
  pb_istream_read_string(&stream, entry->uuids.parent, sizeof(entry->uuids.parent), 8);
  pb_istream_read_varlong(&stream, &entry->timestamp, 9);

  if (type != 0 || subtype != 0) {
    return false;
  }

  unsigned char subbuffer[512] = {0};
  int subsize = pb_istream_read_buffer(&stream, subbuffer, sizeof(subbuffer), 16);

  return TS3BookmarkEntry_deserialize(entry, subbuffer, subsize);
}

static int TS3BookmarkEntry_serialize(
    struct TS3BookmarkEntry *entry,
    unsigned char *buffer, int size)
{
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, size);

  pb_ostream_write_string(&stream, entry->bookmarkName, 1);
  pb_ostream_write_string(&stream, entry->serverAddress, 2);
  pb_ostream_write_varint(&stream, entry->serverPort, 3);
  pb_ostream_write_string(&stream, entry->nickname, 4);

  pb_ostream_write_string(&stream, "", 5); // phonetic nickname
  pb_ostream_write_string(&stream, "", 6);

  pb_ostream_write_string(&stream, entry->serverPassword, 7);

  pb_ostream_write_string(&stream, "", 8); // default channel
  pb_ostream_write_string(&stream, "", 9); // default channel password
  pb_ostream_write_varint(&stream, 0,  10);
  pb_ostream_write_string(&stream, "", 11);
  pb_ostream_write_string(&stream, "", 12);
  pb_ostream_write_string(&stream, "", 13);
  pb_ostream_write_varint(&stream, 0,  14);
  pb_ostream_write_string(&stream, "", 15);
  pb_ostream_write_string(&stream, "", 17);
  pb_ostream_write_varint(&stream, 0,  18);
  pb_ostream_write_varint(&stream, 0,  19);
  pb_ostream_write_varint(&stream, 0,  22);

  return stream.bytes_written;
}

int TS3Bookmarks_serialize(
    struct TS3BookmarkEntry *entry,
    unsigned char *buffer, int size)
{
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, size);
  const char *spacer = "ffffffff-ffff-ffff-ffff-ffffffffffff";

  pb_ostream_write_string(&stream, entry->uuids.self, 2);
  pb_ostream_write_varint(&stream, 0, 4);
  pb_ostream_write_string(&stream, spacer, 5);
  pb_ostream_write_varint(&stream, 0, 6);
  pb_ostream_write_string(&stream, entry->uuids.folder, 7);
  pb_ostream_write_string(&stream, entry->uuids.parent, 8);
  pb_ostream_write_varint(&stream, entry->timestamp, 9);

  unsigned char subbuffer[512] = {0};
  size_t subsize = TS3BookmarkEntry_serialize(entry, subbuffer, sizeof(subbuffer));

  pb_ostream_write_buffer(&stream, subbuffer, subsize, 16);
  return stream.bytes_written;
}
