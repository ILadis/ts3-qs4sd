
#include "ts3remote.h"
#include "plugin.h"

#define length(array) (sizeof(array)/sizeof(array[0]))

static void TS3Remote_resetClientList(struct TS3Remote *remote);
static void TS3Remote_resetCursor(struct TS3Remote *remote);
static void TS3Remote_resetBrowser(struct TS3Remote *remote);

struct TS3Remote* TS3Remote_getInstance(uint64 handle) {
  static struct TS3Remote ts3remote = {0};

  if (ts3remote.handle == 0) {
    ts3remote.handle = handle;
    return &ts3remote;
  }

  if (ts3remote.handle == handle || handle == 0) {
    return &ts3remote;
  }

  return NULL;
}

static void TS3Remote_freeMemory(void **value) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  if (*value != NULL) {
    ts3->freeMemory(*value);
    *value = NULL;
  }
}

static void TS3Server_reset(struct TS3Server *server) {
  server->status = 0;
  TS3Remote_freeMemory((void **) &server->name);
}

static void TS3Server_update(struct TS3Server *server, struct TS3Remote* remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Server_reset(server);
  TS3Remote_guardHandleIsset(remote);

  ts3->getServerVariableAsString(remote->handle, VIRTUALSERVER_NAME, &server->name);
  ts3->getConnectionStatus(remote->handle, &server->status);
}

static void TS3Client_reset(struct TS3Client *client) {
  client->id = 0;
  client->outputMuted = false;
  client->inputMuted = false;
  TS3Remote_freeMemory((void **) &client->nickname);
}

static void TS3Client_update(struct TS3Client *client, anyID id, struct TS3Remote* remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  int flag = 0;

  TS3Client_reset(client);
  TS3Remote_guardHandleIsset(remote);

  client->id = id;
  ts3->getClientVariableAsString(remote->handle, client->id, CLIENT_NICKNAME, &client->nickname);

  ts3->getClientVariableAsInt(remote->handle, client->id, CLIENT_INPUT_MUTED, &flag);
  client->inputMuted = flag > 0;

  ts3->getClientVariableAsInt(remote->handle, client->id, CLIENT_OUTPUT_MUTED, &flag);
  client->outputMuted = flag > 0;
}

static void TS3Channel_reset(struct TS3Channel *channel) {
  channel->id = 0;
  TS3Remote_freeMemory((void **) &channel->name);
}

static void TS3Channel_update(struct TS3Channel *channel, uint64 id, struct TS3Remote* remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Channel_reset(channel);
  TS3Remote_guardHandleIsset(remote);

  channel->id = id;

  if (id == 0) {
    ts3->getServerVariableAsString(remote->handle, VIRTUALSERVER_NAME, &channel->name);
  } else {
    ts3->getChannelVariableAsString(remote->handle, channel->id, CHANNEL_NAME, &channel->name);
  }
}

static void TS3Bookmark_reset(struct TS3Bookmark *bookmark) {
  TS3Remote_freeMemory((void **) &bookmark->name);
  TS3Remote_freeMemory((void **) &bookmark->uuid);
}

static void TS3Bookmark_update(struct TS3Bookmark *bookmark, struct PluginBookmarkItem *item) {
  TS3Bookmark_reset(bookmark);

  if (item != NULL) {
    bookmark->name = item->name;
    bookmark->uuid = item->uuid;
  }
}

static void TS3Remote_iterateBookmarkList(struct TS3Remote *remote, struct PluginBookmarkList *list) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  struct TS3Server *server = &remote->server;

  for (int i = 0; i < list->itemcount; i++) {
    struct PluginBookmarkItem *item = &list->items[i];

    if (item->isFolder) {
      ts3->freeMemory(item->name);
      TS3Remote_iterateBookmarkList(remote, item->folder);
    }
    else if (server->numBookmarks < length(server->bookmarks)) {
      int j = server->numBookmarks++;
      TS3Bookmark_update(&server->bookmarks[j], item);
    }
  }
}

bool TS3Remote_loadBookmarks(struct TS3Remote *remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  struct TS3Server *server = &remote->server;

  server->numBookmarks = 0;
  for (int i = 0; i < length(server->bookmarks); i++) {
    TS3Bookmark_reset(&server->bookmarks[i]);
  }

  struct PluginBookmarkList *list = NULL;
  if (ts3->getBookmarkList(&list) == 0) {
    TS3Remote_iterateBookmarkList(remote, list);
    ts3->freeMemory(list);

    return true;
  }

  return false;
}

bool TS3Remote_connectBookmark(struct TS3Remote *remote, const char *uuid) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_resetConnection(remote);

  if (uuid != NULL) {
    int result = ts3->guiConnectBookmark(PLUGIN_CONNECT_TAB_CURRENT, uuid, &remote->handle);
    return result == 0;
  }

  return false;
}

void TS3Remote_updateConnection(struct TS3Remote *remote) {
  struct TS3Server *server = &remote->server;
  TS3Server_update(server, remote);
}

bool TS3Remote_closeConnection(struct TS3Remote *remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_guardHandleIsset(remote, false);

  int result = ts3->stopConnection(remote->handle, "");
  TS3Remote_resetConnection(remote);

  return result == 0;
}

void TS3Remote_resetConnection(struct TS3Remote *remote) {
  struct TS3Server *server = &remote->server;

  remote->handle = 0;
  TS3Server_reset(server);

  TS3Remote_resetClientList(remote);
  TS3Remote_resetCursor(remote);
  TS3Remote_resetBrowser(remote);
}

void TS3Remote_updateClient(struct TS3Remote *remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  struct TS3Client *client = &remote->client;

  TS3Client_reset(client);
  TS3Remote_guardHandleIsset(remote);

  anyID clientId = 0;
  ts3->getClientID(remote->handle, &clientId);

  TS3Client_update(client, clientId, remote);
}

void TS3Remote_updateClientWithId(struct TS3Remote *remote, anyID clientId) {
  struct TS3Client *client = &remote->client;
  struct TS3ClientList *list = &remote->clientList;
  struct TS3Cursor *cursor = &remote->cursor;

  if (client->id == clientId) {
    TS3Client_update(client, clientId, remote);
  }

  for (int i = 0; i < list->size; i++) {
    client = &list->items[i].client;
    if (client->id == clientId) {
      TS3Client_update(client, clientId, remote);
      break;
    }
  }

  for (int i = 0; i < cursor->numClients; i++) {
    client = &cursor->clients[i];
    if (client->id == clientId) {
      TS3Client_update(client, clientId, remote);
      break;
    }
  }
}

void TS3Remote_updateClientList(struct TS3Remote *remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  struct TS3ClientList *list = &remote->clientList;

  TS3Remote_resetClientList(remote);
  TS3Remote_guardHandleIsset(remote);

  anyID *clientIds = NULL;
  ts3->getClientList(remote->handle, &clientIds);

  list->size = 0;
  for (int i = 0; clientIds[i] && i < length(list->items); i++) {
    list->size++;

    uint64 channelId = 0;
    ts3->getChannelOfClient(remote->handle, clientIds[i], &channelId);

    TS3Client_update(&list->items[i].client, clientIds[i], remote);
    TS3Channel_update(&list->items[i].channel, channelId, remote);
  }
}

static void TS3Remote_resetClientList(struct TS3Remote *remote) {
  struct TS3ClientList *list = &remote->clientList;

  list->size = 0;
  for (int i = 0; i < length(list->items); i++) {
    TS3Client_reset(&list->items[i].client);
    TS3Channel_reset(&list->items[i].channel);
  }
}

int TS3Remote_openClientAvatar(struct TS3Remote *remote, anyID clientId) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  char filepath[2048];

  int result = ts3->getAvatar(remote->handle, clientId, filepath, length(filepath));
  if (result == 0) {
    // avatar not yet downloaded
    if(strlen(filepath) <= 0) {
      return 0;
    }

    return open(filepath, O_RDONLY);
  }

  return -1;
}

void TS3Remote_muteInput(struct TS3Remote *remote, bool mute) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_guardHandleIsset(remote);

  ts3->setClientSelfVariableAsInt(remote->handle, CLIENT_INPUT_MUTED, mute ? 1 : 0);
  ts3->flushClientSelfUpdates(remote->handle, NULL);
}

void TS3Remote_muteOutput(struct TS3Remote *remote, bool mute) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_guardHandleIsset(remote);

  ts3->setClientSelfVariableAsInt(remote->handle, CLIENT_OUTPUT_MUTED, mute ? 1 : 0);
  ts3->flushClientSelfUpdates(remote->handle, NULL);
}

void TS3Remote_setAfk(struct TS3Remote *remote, bool afk) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_guardHandleIsset(remote);

  ts3->setClientSelfVariableAsInt(remote->handle, CLIENT_AWAY, afk ? 1 : 0);
  ts3->flushClientSelfUpdates(remote->handle, NULL);
}

void TS3Remote_shouldTalk(struct TS3Remote *remote, bool talk) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_guardHandleIsset(remote);

  ts3->setClientSelfVariableAsInt(remote->handle, CLIENT_INPUT_DEACTIVATED, talk ? INPUT_ACTIVE : INPUT_DEACTIVATED);
  ts3->flushClientSelfUpdates(remote->handle, NULL);
}

void TS3Remote_setCursorToSelf(struct TS3Remote *remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_guardHandleIsset(remote);

  anyID selfId = 0;
  ts3->getClientID(remote->handle, &selfId);

  uint64 channelId = 0;
  ts3->getChannelOfClient(remote->handle, selfId, &channelId);

  TS3Remote_setCursorToChannel(remote, channelId);
}

void TS3Remote_setCursorToChannel(struct TS3Remote *remote, uint64 channelId) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  struct TS3Cursor *cursor = &remote->cursor;
  struct TS3Channel *channel = &cursor->channel;

  TS3Remote_resetCursor(remote);
  TS3Channel_update(channel, channelId, remote);
  TS3Remote_guardHandleIsset(remote);

  anyID *clientIds = NULL;
  ts3->getChannelClientList(remote->handle, channel->id, &clientIds);

  cursor->numClients = 0;
  for (int i = 0; clientIds[i] && i < length(cursor->clients); i++) {
    cursor->numClients++;
    TS3Client_update(&cursor->clients[i], clientIds[i], remote);
  }

  TS3Remote_freeMemory((void **) &clientIds);
}

void TS3Remote_updateCursor(struct TS3Remote *remote) {
  struct TS3Cursor *cursor = &remote->cursor;
  struct TS3Channel *channel = &cursor->channel;

  uint64 id = channel->id;
  if (id != 0) {
    TS3Remote_setCursorToChannel(remote, id);
  }
}

bool TS3Remote_joinCursor(struct TS3Remote *remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  struct TS3Client *client = &remote->client;
  struct TS3Cursor *cursor = &remote->cursor;
  struct TS3Channel *channel = &cursor->channel;

  TS3Remote_guardHandleIsset(remote, false);

  uint64 id = channel->id;
  if (id != 0) {
    int result = ts3->requestClientMove(remote->handle, client->id, id, "", NULL);
    return result == 0;
  }

  return false;
}

static void TS3Remote_resetCursor(struct TS3Remote *remote) {
  struct TS3Cursor *cursor = &remote->cursor;
  struct TS3Channel *channel = &cursor->channel;

  TS3Channel_reset(channel);

  cursor->numClients = 0;
  for (int i = 0; i < length(cursor->clients); i++) {
    TS3Client_reset(&cursor->clients[i]);
  }
}

void TS3Remote_setBrowserToSelf(struct TS3Remote *remote) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();

  TS3Remote_guardHandleIsset(remote);

  anyID selfId = 0;
  ts3->getClientID(remote->handle, &selfId);

  uint64 channelId = 0;
  ts3->getChannelOfClient(remote->handle, selfId, &channelId);

  TS3Remote_setBrowserToChannel(remote, channelId);
}

void TS3Remote_setBrowserToChannel(struct TS3Remote *remote, uint64 channelId) {
  struct TS3Functions *ts3 = ts3plugin_getFunctionPointers();
  struct TS3Browser *browser = &remote->browser;
  struct TS3Channel *channel = &browser->channel;

  TS3Remote_guardHandleIsset(remote);

  if (channelId == -1) {
    ts3->getParentChannelOfChannel(remote->handle, channel->id, &channelId);
  }

  TS3Remote_resetBrowser(remote);
  TS3Channel_update(channel, channelId, remote);

  uint64 *channelIds = NULL;
  ts3->getChannelList(remote->handle, &channelIds);

  browser->numChilds = 0;
  for (int i = 0, j = 0; channelIds[i] && j < length(browser->childs); i++) {
    uint64 parentChannelId = 0;
    ts3->getParentChannelOfChannel(remote->handle, channelIds[i], &parentChannelId);

    if (parentChannelId == channelId) {
      browser->numChilds++;
      TS3Channel_update(&browser->childs[j++], channelIds[i], remote);
    }
  }

  TS3Remote_freeMemory((void **) &channelIds);
}

void TS3Remote_updateBrowser(struct TS3Remote *remote) {
  struct TS3Browser *browser = &remote->browser;
  struct TS3Channel *channel = &browser->channel;

  uint64 id = channel->id;
  if (id != 0) {
    TS3Remote_setBrowserToChannel(remote, id);
  }
}

static void TS3Remote_resetBrowser(struct TS3Remote *remote) {
  struct TS3Browser *browser = &remote->browser;
  struct TS3Channel *channel = &browser->channel;

  TS3Channel_reset(channel);

  browser->numChilds = 0;
  for (int i = 0; i < length(browser->childs); i++) {
    TS3Channel_reset(&browser->childs[i]);
  }
}
