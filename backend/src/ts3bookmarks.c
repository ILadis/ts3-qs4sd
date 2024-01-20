
#include "log.h"
#include "ts3bookmarks.h"
#include "protobuf.h"

void TS3Bookmarks_test() {
  struct TS3BookmarkEntry entry = {
    .guid = "f861381c-dc98-021d-83b6-542b2f80344c",
    .puid = "20831eae-bc63-5d30-215c-ff7e9a946e17",
    .nickname = "Ladis",
    .bookmarkName = "New Server",
    .serverAddress = "ladi.dev",
    .serverPort = 1337,
    .timestamp = 1705793777,
  };

  unsigned char buffer[2048] = {0};
  TS3Bookmarks_serialize(&entry, buffer, sizeof(buffer));

  struct TS3BookmarkManager *manager = TS3BookmarkManager_getInstance();
  if (!TS3BookmarkManager_openDatabase(manager, "/home/ladis/.ts3client/settings.db")) {
    Logger_errorLog("Could not open database!");
  }

  TS3BookmarkManager_lastBookmarkUUID(manager, NULL);
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
    return false;
  }

  int result = sqlite3_open(dbpath, &manager->db);
  if (result != SQLITE_OK) {
    sqlite3_close(manager->db);
    manager->db = NULL;
    return false;
  }

  return true;
}

bool TS3BookmarkManager_lastBookmarkUUID(
    struct TS3BookmarkManager *manager,
    char *uuid)
{
  sqlite3_stmt *stmt;
  int result = sqlite3_prepare(manager->db, "SELECT value FROM ProtobufItems", -1, &stmt, NULL);

  if (result != SQLITE_OK) {
    return false;
  }

  struct TS3BookmarkEntry entry = {0};

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *buffer = sqlite3_column_blob(stmt, 0);
    int size = sqlite3_column_bytes(stmt, 0);

    bool result = TS3Bookmarks_deserialize(&entry, buffer, size);
    Logger_debugLog("Got GUID: %s", entry.guid);
    Logger_debugLog("Got PUID: %s", entry.puid);
    Logger_debugLog("Got timestamp: %ld", entry.timestamp);

    if (result) {
      Logger_debugLog("Got bookmarkName: %s", entry.bookmarkName);
      Logger_debugLog("Got serverAddress: %s", entry.serverAddress);
      Logger_debugLog("Got serverPort: %d", entry.serverPort);
      Logger_debugLog("Got nickname: %s", entry.nickname);
      Logger_debugLog("Got serverPassword: %s", entry.serverPassword);
    }
  }

  sqlite3_finalize(stmt);
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

  pb_istream_read_string(&stream, entry->guid, sizeof(entry->guid), 2);
  pb_istream_read_varint(&stream, &type, 4);
  pb_istream_read_buffer(&stream, NULL, 0, 5);
  pb_istream_read_varint(&stream, &subtype, 6);
  pb_istream_read_buffer(&stream, NULL, 0, 7);
  pb_istream_read_string(&stream, entry->puid, sizeof(entry->puid), 8);
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

  pb_ostream_write_string(&stream, entry->guid, 2);
  pb_ostream_write_varint(&stream, 0, 4);
  pb_ostream_write_string(&stream, spacer, 5);
  pb_ostream_write_varint(&stream, 0, 6);
  pb_ostream_write_string(&stream, "", 7);
  pb_ostream_write_string(&stream, entry->puid, 8);
  pb_ostream_write_varint(&stream, entry->timestamp, 9);

  unsigned char subbuffer[512] = {0};
  size_t subsize = TS3BookmarkEntry_serialize(entry, subbuffer, sizeof(subbuffer));

  pb_ostream_write_buffer(&stream, subbuffer, subsize, 16);
  return stream.bytes_written;
}
