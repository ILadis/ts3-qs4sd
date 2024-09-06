
import { createElement as $ } from 'react';
import { definePlugin } from 'decky-frontend-lib';

import { App } from './app.js';
import { Client } from './client.js';
import { TS3LogoIcon } from './components.js';
import { debounce } from './utils.js'

export default definePlugin(serverAPI => {
  const client = new Client('http://127.0.0.1:52259/api', serverAPI);

  client.setAudioOutputVolume = debounce(client.setAudioOutputVolume.bind(client));

  return {
    content: $(App, { client }),
    icon: $(TS3LogoIcon),
  };
});
