
import { createElement as $, useState, useEffect } from 'react';
import { definePlugin } from 'decky-frontend-lib';

import { Client } from './client';
import { TS3QuickAccessPanel, TS3LogoIcon } from './components';
import { retry, dispatch } from './utils.js'

function App({ client }) {
  const [content, setContent] = useState(null);
  const [bookmarks, setBookmarks] = useState([]);
  const [server, setServer] = useState(null);
  const [channels, setChannels] = useState([]);
  const [self, setSelf] = useState(null);
  const [outputs, setOutputs] = useState([]);

  let queue = Promise.resolve();

  function connectTo(bookmark) {
    client.connect(bookmark.uuid);
  }

  async function joinChannel(channel) {
    await client.moveCursor(channel);
    await client.joinCursor();
  }

  function disconnect() {
    client.disconnect();
  }

  async function toggleMute(device) {
    const state = !self.muted[device];
    await client.muteClient(self.id, device, state);

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

    setOutputs((outputs) => outputs.map(output => {
      if (output.index === index) {
        output.volume = volume;
        queue = queue.then(() => client.setAudioOutputVolume(index, volume));
      }
      return output;
    }));
  }

  async function restoreState() {
    const server = await retry(() => client.getServer(), Infinity);

    if (server.status == 0) {
      refreshBookmarksState();
    } else {
      refreshDashboardState();
    }
  }

  function handleEvent(event) {
    const states = {
      'CONNECTION_STATE_CONNECTED': refreshBookmarksState,
      'CONNECTION_STATE_DISCONNECTED': refreshBookmarksState,
      'CLIENT_LIST_CHANGED': refreshDashboardState,
    };

    states[event.type]?.();
  }

  function listenEvents() {
    const events = client.listenEvents();
    let promise = null;

    async function nextEvent() {
      promise = events.next().value;
      promise.then(handleEvent).then(nextEvent);
    }

    dispatch(nextEvent);

    return () => promise?.cancel();
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
      self,
      outputs,
      // actions
      connectTo,
      joinChannel,
      disconnect,
      toggleMute,
      toggleOutputs,
      // events
      outputChanged,
    })
  );
}

export default definePlugin(server => {
  const client = new Client('http://localhost:8000/api');

  return {
    content: $(App, { client }),
    icon: $(TS3LogoIcon),
  };
});
