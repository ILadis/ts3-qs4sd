
import { createElement as $, useState, useEffect } from 'react';
import { definePlugin } from 'decky-frontend-lib';

import { Client } from './client';
import { TS3QuickAccessPanel, TS3ChannelPasswordPrompt, TS3LogoIcon } from './components';
import { retry, debounce } from './utils.js'

function App({ client }) {
  const [content, setContent] = useState(null);
  const [bookmarks, setBookmarks] = useState([]);
  const [server, setServer] = useState(null);
  const [channels, setChannels] = useState([]);
  const [browser, setBrowser] = useState([]);
  const [parent, setParent] = useState([]);
  const [self, setSelf] = useState(null);
  const [outputs, setOutputs] = useState([]);

  function connectTo(bookmark) {
    client.connect(bookmark.uuid);
  }

  function isSelfInChannel(channel) {
    const byId = ({ id }) => channel.id === id;
    channel = channels.find(byId);

    if (!channel) {
      return false;
    }

    for (let client of channel.clients) {
      if (client.id === self.id) {
        return true;
      }
    }

    return false;
  }

  async function joinChannel(channel) {
    if (channel.hasPassword) {
      var password = await TS3ChannelPasswordPrompt.show();
    }

    await client.moveCursor(channel);
    await client.joinCursor(password);
  }

  function disconnect() {
    client.disconnect();
  }

  async function browseChannels(channel = { id: 0 }) {
    await client.moveBrowser(channel);
    const channels = await client.browseChannels();
    if (channels.length > 0) {
      setParent([channel, ...parent]);
      setBrowser(channels);
    }
  }

  function isBrowserOnRoot() {
    return parent.length <= 1;
  }

  function browseParentChannels() {
    if (!isBrowserOnRoot()) {
      const [_, channel] = parent.splice(0, 2);
      browseChannels(channel);
    }
  }

  async function rebindPttHotkey() {
    await client.rebindPttHotkey();
    await client.getSelf().then(setSelf);
    await client.waitEvent('PTT_HOTKEYS_PRESSED');
    await client.getSelf().then(setSelf);
  }

  async function clearPttHotkey() {
    await client.clearPttHotkey();
    await client.getSelf().then(setSelf);
  }

  async function toggleMute(device) {
    const state = !self.muted[device];
    await client.muteSelf(self.id, device, state);

    setSelf((self) => {
      self.muted[device] = state
      return self;
    });
  }

  async function toggleOutputs() {
    if (outputs.length == 0) {
      const outputs = [];
      for await (let output of client.getAudioOutputs()) {
        outputs.push(output);
      }

      setOutputs(outputs);
    } else {
      setOutputs([]);
    }
  }

  function outputChanged(output, volume) {
    const index = output.index;
    client.setAudioOutputVolume(index, volume);

    setOutputs((outputs) => outputs.map(output => {
      if (output.index === index) {
        output.volume = volume;
      }
      return output;
    }));
  }

  async function restoreState() {
    const status = await client.getStatus();

    if (status.installed) {
      await client.start();
      const server = await retry(() => client.getServer(), Infinity);

      if (server.status == 0) {
        refreshBookmarksState();
      } else {
        refreshDashboardState();
      }
    } else {
      setContent('setup');
    }
  }

  async function handleEvents(events) {
    const states = {
      'CONNECTION_STATE_CONNECTED': refreshBookmarksState,
      'CONNECTION_STATE_DISCONNECTED': refreshBookmarksState,
      'CLIENT_LIST_CHANGED': refreshDashboardState,
    };

    for await (let event of events) {
      states[event.type]?.();
    }
  }

  function listenEvents() {
    const events = client.listenEvents();
    handleEvents(events);

    return () => client.closeEvents();
  }

  async function refreshBookmarksState() {
    const bookmarks = [];
    for await (let bookmark of client.getBookmarks()) {
      bookmarks.push(bookmark);
    }

    setBookmarks(bookmarks);
    setContent('bookmarks');
  }

  async function refreshDashboardState() {
    const server = await client.getServer();
    setServer(server);

    const self = await client.getSelf();
    setSelf(self);

    const channels = await client.listChannels();
    setChannels(channels);

    browseChannels();
    setContent('dashboard');
  }

  useEffect(() => (restoreState(), listenEvents()), []);

  return (
    $(TS3QuickAccessPanel, {
      // states
      content,
      bookmarks,
      server,
      channels,
      browser,
      self,
      outputs,
      // actions
      setContent,
      connectTo,
      isSelfInChannel,
      joinChannel,
      disconnect,
      browseChannels,
      isBrowserOnRoot,
      browseParentChannels,
      rebindPttHotkey,
      clearPttHotkey,
      toggleMute,
      toggleOutputs,
      // events
      outputChanged,
    })
  );
}

export default definePlugin(serverAPI => {
  const client = new Client('http://127.0.0.1:52259/api', serverAPI);

  client.setAudioOutputVolume = debounce(client.setAudioOutputVolume.bind(client));

  return {
    content: $(App, { client }),
    icon: $(TS3LogoIcon),
  };
});
