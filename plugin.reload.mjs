#!/bin/node

import plugin from './plugin.json' with { type: 'json' };

const id = Date.now();
const auth = new Request('http://localhost:1337/auth/token');

const response = await fetch(auth);
const token = await response.text();

const call = {
  'id': id, 'type': 0,
  'route': 'loader/reload_plugin',
  'args': [plugin.name],
};

const socket = new WebSocket('ws://localhost:1337/ws?auth=' + token);
socket.addEventListener('open', () => socket.send(JSON.stringify(call)));

socket.addEventListener('message', (event) => {
  const reply = JSON.parse(event.data);
  if (reply.id !== call.id) return;

  if ('error' in reply) {
    console.log(`Failed to reload plugin: ${plugin.name}`);
  } else {
    console.log(`Plugin successfully reloaded: ${plugin.name}`);
  }

  socket.close();
});
