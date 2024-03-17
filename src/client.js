
import { retry, sleep, fətch, generate } from './utils.js'

export function Client(endpoint, api) {
  let self = new URL(import.meta.url);
  this.events = new Array();
  this.endpoint = endpoint || (self.origin + '/api');
  this.api = api;
}

Client.prototype.getStatus = async function() {
  let response = await this.api.callPluginMethod('status', { });
  if (!response.success) {
    throw new Error('failed to fetch status');
  }

  let result = response.result;

  return {
    'running': Boolean(result['running']),
    'installed': Boolean(result['installed']),
  };
};

Client.prototype.start = async function() {
  let response = await this.api.callPluginMethod('start', { });
  if (!response.success) {
    throw new Error('failed to start instance');
  }
}

Client.prototype.connect = async function(uuid) {
  let url = this.endpoint + '/server/connect';
  let body = JSON.stringify({
    'uuid': String(uuid)
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error(`failed to connect to server ${uuid}`);
  }
};

Client.prototype.disconnect = async function() {
  let url = this.endpoint + '/server/disconnect';

  let request = new Request(url, {
    method: 'POST'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to disconnect from server');
  }
};

Client.prototype.getServer = async function() {
  let url = this.endpoint + '/server';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to fetch server');
  }

  let server = await response.json();

  return {
    'name': String(server['name']),
    'status': Number(server['status']),
  };
};

Client.prototype.getBookmarks = async function*() {
  let url = this.endpoint + '/server';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to fetch bookmarks');
  }

  let server = await response.json();
  let bookmarks = server['bookmarks'];

  for (let bookmark of bookmarks) {
    yield {
      'name': String(bookmark['name']),
      'uuid': String(bookmark['uuid']),
    };
  }
};

Client.prototype.getSelf = async function() {
  let url = this.endpoint + '/self';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to fetch self');
  }

  let client = await response.json();

  return {
    'id': Number(client['client_id']),
    'nickname': String(client['client_nickname']),
    'avatar': this.endpoint + '/clients/' + client['client_id'] + '/avatar',
    'muted': {
      'input': Boolean(client['input_muted']),
      'output': Boolean(client['output_muted']),
    },
    'ptt': {
      'state': String(client['ptt_state']),
      'hotkey': String(client['ptt_hotkey']),
    }
  };
};

Client.prototype.rebindPttHotkey = async function() {
  let url = this.endpoint + '/self/ptt/rebind';

  let request = new Request(url, {
    method: 'POST'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to rebind ptt hotkey');
  }
};

Client.prototype.clearPttHotkey = async function() {
  let url = this.endpoint + '/self/ptt/clear';

  let request = new Request(url, {
    method: 'POST'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to clear ptt hotkey');
  }
};

Client.prototype.muteSelf = async function(id, device, mute) {
  let url = this.endpoint + '/self/' + (mute ? 'mute' : 'unmute');
  let body = JSON.stringify({
    'client_id': Number(id),
    'device': String(device),
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error(`failed to mute/unmute ${device} device of client ${id}`);
  }
};

Client.prototype.getAudioOutputs = async function*() {
  let url = this.endpoint + '/audio/outputs';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to fetch audio outputs');
  }

  let outputs = await response.json();

  for (let output of outputs) {
    yield {
      'index': Number(output['index']),
      'name': String(output['name']),
      'volume': Number(output['volume']) / 100,
    };
  }
};

Client.prototype.setAudioOutputVolume = async function(index, volume) {
  let url = this.endpoint + '/audio/outputs/volume';
  let body = JSON.stringify({
    'index': Number(index),
    'volume': Number(volume) * 100,
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to set audio output volume');
  }
};

Client.prototype.listChannels = async function() {
  let url = this.endpoint + '/clients';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to fetch clients');
  }

  let clients = await response.json();
  let channels = [];

  for (let client of clients) {
    let id = Number(client['channel_id']);
    let channel = channels.find(c => c.id == id);

    if (!channel) {
      channels.push(channel = {
        'id': Number(client['channel_id']),
        'name': String(client['channel_name']),
        'maxClients': Number(client['channel_max_clients']),
        'hasPassword': Boolean(client['channel_has_password']),
        'clients': Array(),
      });
    }

    channel.clients.push({
      'id': Number(client['client_id']),
      'nickname': String(client['client_nickname']),
      'avatar': this.endpoint + '/clients/' + client['client_id'] + '/avatar',
      'muted': {
        'input': Boolean(client['input_muted']),
        'output': Boolean(client['output_muted']),
      }
    });
  }

  return channels;
};

Client.prototype.moveCursor = async function(channel) {
  let url = this.endpoint + '/cursor/move';
  let body = JSON.stringify({
    'channel_id': Number(channel.id)
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error(`failed to move cursor to channel ${channel.id}`);
  }
};

Client.prototype.joinCursor = async function(password) {
  let url = this.endpoint + '/cursor/join';
  let body = JSON.stringify({
    'password': String(password || '')
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to join channel the cursor points to');
  }
};

Client.prototype.browseChannels = async function() {
  let url = this.endpoint + '/browser';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error('failed to browse channels');
  }

  let browser = await response.json();
  let channels = [];

  for (let channel of browser['sub_channels']) {
    channels.push({
      'id': Number(channel['channel_id']),
      'name': String(channel['channel_name']),
      'order': Number(channel['channel_order']),
      'maxClients': Number(channel['channel_max_clients']),
      'hasPassword': Boolean(channel['channel_has_password']),
    });
  }

  let sorted = [];
  let nextId = 0;

  while (true) {
    let channel = channels.find(channel => channel.order === nextId);

    if (!channel) break;
    else delete channel['order'];

    sorted.push(channel);
    nextId = channel.id;
  }

  return sorted;
};

Client.prototype.moveBrowser = async function(channel) {
  let url = this.endpoint + '/browser/move';
  let body = JSON.stringify({
    'channel_id': Number(channel.id)
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await retry(() => fətch(request.clone()));
  if (!response.ok) {
    throw new Error(`failed to move browser to channel ${channel.id}`);
  }
};

Client.prototype.listenEvents = function() {
  const url = this.endpoint + '/events';

  var { iterator, take } = generate();
  var source = connect();

  function connect() {
    source?.close();
    source = new EventSource(url);
    source.onmessage = consume;
    source.onerror = reconnect;
    return source;
  }

  function reconnect() {
    sleep(500).then(connect);
  }

  function consume(event) {
    let data = JSON.parse(event.data);
    take(data);
  }

  this.events.push(() => source);
  return iterator();
};

Client.prototype.waitEvent = function(type) {
  const url = this.endpoint + '/events';
  const source = new EventSource(url);

  function consume(resolve, event) {
    let data = JSON.parse(event.data);
    if (data.type == type) {
      source.close();
      resolve(data);
    }
  }

  this.events.push(() => source);
  return new Promise((resolve, reject) => {
    source.onmessage = (event) => consume(resolve, event);
    source.onerror = reject;
  });
};

Client.prototype.closeEvents = function() {
  while (this.events.length > 0) {
    let source = this.events.pop()();
    source?.close();
  }
};
