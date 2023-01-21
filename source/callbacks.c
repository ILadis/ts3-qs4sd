
#include "settings.h"
#include "plugin.h"
#include "ts3remote.h"
#include "paudio.h"

#include "server.h"
#include "api.h"

static struct TS3Functions ts3funcs = {0};
static struct mg_server server = {0};

struct TS3Functions* ts3plugin_getFunctionPointers() {
  return &ts3funcs;
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
  ts3funcs = funcs;
}

struct mg_server* ts3plugin_getServer() {
  return &server;
}

int ts3plugin_init() {
  mg_server_add_handler(&server, mg_handler_not_found());
  mg_server_add_handler(&server, mg_handler_get_server());
  mg_server_add_handler(&server, mg_handler_connect_server());
  mg_server_add_handler(&server, mg_handler_disconnect_server());
  mg_server_add_handler(&server, mg_handler_get_clientlist());
  mg_server_add_handler(&server, mg_handler_get_self());
  mg_server_add_handler(&server, mg_handler_mute_toggle_self());
  mg_server_add_handler(&server, mg_handler_afk_toggle_self());
  mg_server_add_handler(&server, mg_handler_get_cursor());
  mg_server_add_handler(&server, mg_handler_move_cursor());
  mg_server_add_handler(&server, mg_handler_join_cursor());
  mg_server_add_handler(&server, mg_handler_get_browser());
  mg_server_add_handler(&server, mg_handler_move_browser());
  mg_server_add_handler(&server, mg_handler_events());
  mg_server_add_handler(&server, mg_handler_get_audio_outputs());
  mg_server_add_handler(&server, mg_handler_set_audio_output_volume());
  mg_server_add_handler(&server, mg_handler_static_resources());

  bool result = mg_server_start(&server, SERVER_PORT);
  if (!result) {
    return 1;
  }

  return 0;
}

void ts3plugin_shutdown() {
  mg_server_stop(&server);

  struct TS3Remote *remote = TS3Remote_getInstance(0);
  TS3Remote_resetConnection(remote);

  struct PAudio *paudio = PAudio_getInstance();
  PAudio_shutdown(paudio);
}

void ts3plugin_onConnectStatusChangeEvent(
    uint64 serverConnectionHandlerId,
    int newStatus, unsigned int errorNumber)
{
  struct TS3Remote *remote = TS3Remote_getInstance(serverConnectionHandlerId);

  if (remote != NULL) {
    switch (newStatus) {
    case STATUS_DISCONNECTED:
      TS3Remote_resetConnection(remote);
      mg_server_user_event(&server, CONNECTION_STATE_DISCONNECTED, NULL);
      break;

    case STATUS_CONNECTION_ESTABLISHED:
      TS3Remote_updateConnection(remote);
      TS3Remote_updateClient(remote);
      TS3Remote_updateClientList(remote);
      TS3Remote_setCursorToSelf(remote);
      TS3Remote_setBrowserToSelf(remote);
      mg_server_user_event(&server, CONNECTION_STATE_CONNECTED, NULL);
      break;
    }
  }
}

// called once after connection is established and all desired channel subscriptions are completed
void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerId) {
  struct TS3Remote *remote = TS3Remote_getInstance(serverConnectionHandlerId);

  if (remote != NULL) {
    TS3Remote_updateClientList(remote);
    mg_server_user_event(&server, CLIENT_LIST_CHANGED, NULL);
  }
}

void ts3plugin_onClientMoveEvent(
    uint64 serverConnectionHandlerId,
    anyID clientId,
    uint64 oldChannelId, uint64 newChannelId,
    int visibility, const char* moveMessage)
{
  struct TS3Remote *remote = TS3Remote_getInstance(serverConnectionHandlerId);

  if (remote != NULL) {
    struct TS3Client *client = &remote->client;
    struct TS3Cursor *cursor = &remote->cursor;
    struct TS3Browser *browser = &remote->browser;

    // own client (self) changed channel
    if (client->id == clientId) {
      // disconnected from server
      if (newChannelId == 0) {
        return TS3Remote_resetConnection(remote);
      }

      // browser and cursor previously pointed to same channel
      if (browser->channel.id == cursor->channel.id) {
        TS3Remote_setBrowserToSelf(remote);
      }
      TS3Remote_setCursorToSelf(remote);
    }

    // client joined channel cursor points to
    else if (cursor->channel.id == newChannelId) {
      TS3Remote_updateCursor(remote);
    }

    // client left channel cursor points to
    else if (cursor->channel.id == oldChannelId) {
      TS3Remote_updateCursor(remote);
    }

    TS3Remote_updateClientList(remote);
    mg_server_user_event(&server, CLIENT_LIST_CHANGED, NULL);
  }
}

void ts3plugin_onClientDisplayNameChanged(
    uint64 serverConnectionHandlerId,
    anyID clientId,
    const char* displayName,
    const char* uniqueClientIdentifier)
{
  struct TS3Remote *remote = TS3Remote_getInstance(serverConnectionHandlerId);

  if (remote != NULL) {
    TS3Remote_updateClientWithId(remote, clientId);
    mg_server_user_event(&server, CLIENT_LIST_CHANGED, NULL);
  }
}

void ts3plugin_onUpdateClientEvent(
    uint64 serverConnectionHandlerId,
    anyID clientId, anyID invokerId,
    const char* invokerName,
    const char* invokerUniqueIdentifier)
{
  struct TS3Remote *remote = TS3Remote_getInstance(serverConnectionHandlerId);

  if (remote != NULL) {
    TS3Remote_updateClientWithId(remote, clientId);
    mg_server_user_event(&server, CLIENT_LIST_CHANGED, NULL);
  }
}

void paudio_onReady(struct PAudio *paudio) {
  PAudio_updateOutputs(paudio);
}

void paudio_onOutputsChanged(struct PAudio *paudio) {
  mg_server_user_event(&server, AUDIO_OUTPUTS_CHANGED, NULL);
}

void paudio_onError(struct PAudio *paudio, const char *message) {
  // nothing to do, maybe log error message
}
