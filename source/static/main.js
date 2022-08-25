
import { Client } from './client.js';
import * as Views from './views.js';

main();

async function main() {
  let content = await Views.QuickAccessMenu.replaceMenuItem(4, '/static/logo.svg');

  let views = {
    panel: new Views.QuickAccessPanel(),
    bookmarks: new Views.TS3BookmarkList(),
    dashboard: new Views.TS3Dashboard(),
  };

  content.appendChild(views.panel);
  views.panel.addStyles('/static/styles.css');

  let client = new Client(); // for testing use endpoint: 'http://localhost:8000/api'
  let events = client.listenEvents();

  let server = await client.getServer();
  let state = server.status == 0 ? bookmarks : dashboard;

  state(views, client);

  // TODO handle rejected promise (SSE disconnects)
  for await (let event of events) {
    handle(event, views, client);
  }
}

function handle(event, views, client) {
  let handlers = {
    'CONNECTION_STATE_CONNECTED': dashboard,
    'CONNECTION_STATE_DISCONNECTED': bookmarks,
    'CLIENT_LIST_CHANGED': dashboard,
  };

  let handler = handlers[event.type];
  handler?.(views, client);
}

async function bookmarks(views, client) {
  views.panel.setTitle('TeamSpeak 3');
  views.panel.setContent(views.bookmarks);

  let bookmarks = client.getBookmarks();

  let items = views.bookmarks.getBookmarkItems();
  for await (let bookmark of bookmarks) {
    let item = items.next().value || views.bookmarks.addBookmarkItem();
    item.setName(bookmark.name);
    item.onClick = () => client.connect(bookmark.uuid);
  }

  for (let item of items) {
    item.remove();
  }
}

async function dashboard(views, client) {
  views.panel.setTitle('TeamSpeak 3');
  views.panel.setContent(views.dashboard);

  let server = await client.getServer();
  let self = await client.getSelf();
  let channels = await client.listChannels();

  for (let device in self.muted) {
    views.dashboard.setDeviceMuted(device, self.muted[device]);
  }

  views.dashboard.setServerName(server.name);
  views.dashboard.onMuteClick = (_, device) => mute(device);
  views.dashboard.onDisconnectClick = () => client.disconnect();

  views.dashboard.onVolumeSettingsClick = async () => {
    let toggle = false;
    let panels = views.dashboard.getVolumePanels();
    for (let panel of panels) {
      views.dashboard.removeVolumePanel(panel);
      toggle = true;
    }

    if (toggle) return;
    let outputs = client.getAudioOutputs();

    for await (let output of outputs) {
      let panel = views.dashboard.addVolumePanel();
      panel.setLabel(output.name);
      panel.slider.setValue(output.volume);
      panel.slider.onValueChange = (value) => client.setAudioOutputVolume(output.index, value);
    }
  };

  let lists = views.dashboard.getClientLists();
  for (let channel of channels) {
    let list = lists.next().value || views.dashboard.addClientList();
    list.setChannelName(channel.name);
    list.setClientCount(channel.clients.length);

    let items = list.getClientItems();
    for (let client of channel.clients) {
      let item = items.next().value || list.addClientItem();
      item.setNickname(client.nickname);
      item.setStatus('Online'); // TODO implement client status
      item.onClick = () => join(channel);
    }

    for (let item of items) {
      list.removeClientItem(item);
    }
  }

  for (let list of lists) {
    views.dashboard.removeClientList(list);
  }

  async function join(channel) {
    await client.moveCursor(channel);
    await client.joinCursor();
  }

  async function mute(device) {
    let state = !self.muted[device];

    await client.muteClient(self.id, device, state);

    self.muted[device] = state;
    views.dashboard.setDeviceMuted(device, state);
  }
}
