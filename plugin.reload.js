#!/bin/node

import plugin from './plugin.json' assert { type: 'json' };

const auth = new Request('http://localhost:1337/auth/token');

var response = await fetch(auth);
const token = await response.text();

const pluginName = encodeURIComponent(plugin.name);

const reload = new Request(`http://localhost:1337/plugins/${pluginName}/reload`, {
  method: 'POST',
  headers: { 'Authentication': token }
});

var response = await fetch(reload);

if (response.status == 200) {
  console.log(`Plugin successfully reloaded: ${pluginName}`);
} else {
  console.log(`Failed to reload plugin: ${pluginName}`);
}
