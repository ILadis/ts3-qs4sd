
#include "settings.h"
#include "plugin.h"
#include "ts3remote.h"
#include "ts3settings.h"
#include "paudio.h"
#include "sdinput.h"

#include "executor.h"
#include "log.h"

#include "server.h"
#include "api.h"

static struct TS3Functions ts3funcs = {0};
static struct mg_server server = {0};

extern bool PAudio_task();
static struct Executor paudio = Executor_forTask(PAudio_task);

extern bool SDInput_task();
static struct Executor input = Executor_forTask(SDInput_task);

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
  mg_server_add_handler(&server, mg_handler_get_clientavatar());
  mg_server_add_handler(&server, mg_handler_get_self());
  mg_server_add_handler(&server, mg_handler_alter_ptt());
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
  mg_server_add_handler(&server, mg_handler_get_audio_inputs());
  mg_server_add_handler(&server, mg_handler_set_current_audio_input());

  bool result = mg_server_start(&server, SERVER_PORT);
  if (!result) {
    return 1;
  }

  struct TS3Remote *remote = TS3Remote_getInstance(0);
  TS3Settings_load(remote);

  Executor_start(&paudio);
  Executor_start(&input);

  return 0;
}

void ts3plugin_shutdown() {
  mg_server_stop(&server);

  struct TS3Remote *remote = TS3Remote_getInstance(0);
  TS3Settings_save(remote);
  TS3Remote_resetConnection(remote);

  Executor_stop(&paudio);
  Executor_stop(&input);

  struct PAudio *paudio = PAudio_getInstance();
  PAudio_shutdown(paudio);

  struct SDInput *input = SDInput_getInstance();
  SDInput_closeDevice(input);
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
      TS3Settings_save(remote);
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
  PAudio_updateOutputStreams(paudio);

  PAudio_updateInputDevices(paudio);
  PAudio_updateInputStreams(paudio);
}

void paudio_onOutputStreamsChanged(struct PAudio *paudio) {
  mg_server_user_event(&server, AUDIO_OUTPUTS_CHANGED, NULL);
}

void paudio_onInputStreamsChanged(struct PAudio *paudio) {
  // nothing to do, maybe implement user event later
}

void paudio_onInputDevicesChanged(struct PAudio *paudio) {
  mg_server_user_event(&server, AUDIO_INPUTS_CHANGED, NULL);
}

void paudio_onError(struct PAudio *paudio, const char *message) {
  // nothing to do, maybe log error message
}

void sdinput_onUpdate(struct SDInput *input) {
  static bool talk = false;

  enum SDInputKey pttHotkey;
  enum SDInputKey pttHotkeys[] = {
    SDINPUT_KEY_L4,
    SDINPUT_KEY_L5,
    SDINPUT_KEY_R4,
    SDINPUT_KEY_R5,
  };

  struct TS3Remote *remote = TS3Remote_getInstance(0);

  // enable/disable talking by hotkey
  if (SDInputKey_byId(&pttHotkey, remote->pttHotkey)) {
    const char *pttHotkeyName = SDInputKey_getName(pttHotkey);

    if (SDInput_isKeyHeld(input, pttHotkey)) {
      if (!talk) {
        Logger_infoLog("%s button is held: PTT active", pttHotkeyName);
        TS3Remote_shouldTalk(remote, talk = true);
      }
    }
    if (SDInput_isKeyReleased(input, pttHotkey)) {
      if (talk) {
        Logger_infoLog("%s button is NOT held: PTT inactive", pttHotkeyName);
        TS3Remote_shouldTalk(remote, talk = false);
      }
    }
  }

  // notify and rebind hotkeys for PTT
  bool notified = false;
  bool rebind = remote->pttHotkey == TS3_PTT_HOTKEY_REBIND;

  for (int i = 0; i < length(pttHotkeys); i++) {
    enum SDInputKey key = pttHotkeys[i];

    if (SDInput_hasKeyChanged(input, key) && !notified) {
      mg_server_user_event(&server, PTT_HOTKEYS_PRESSED, NULL);
      notified = true;
    }

    if (SDInput_isKeyHeld(input, key) && rebind) {
      TS3Remote_setPttHotkey(remote, (int) key);
      rebind = false;
    }
  }
}
