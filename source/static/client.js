
export function Client(endpoint) {
  let self = new URL(import.meta.url);
  this.endpoint = endpoint || (self.origin + '/api');
}

Client.prototype.connect = async function(uuid) {
  let url = this.endpoint + '/server/connect';
  let body = JSON.stringify({
    'uuid': String(uuid)
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await fetch(request);
  if (!response.ok) {
    throw new Error(`failed to connect to server ${uuid}`);
  }
};

Client.prototype.disconnect = async function() {
  let url = this.endpoint + '/server/disconnect';

  let request = new Request(url, {
    method: 'POST'
  });

  let response = await fetch(request);
  if (!response.ok) {
    throw new Error('failed to disconnect from server');
  }
};

Client.prototype.getServer = async function() {
  let url = this.endpoint + '/server';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await fetch(request);
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

  let response = await fetch(request);
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

  let response = await fetch(request);
  if (!response.ok) {
    throw new Error('failed to fetch self');
  }

  let client = await response.json();

  return {
    'id': Number(client['client_id']),
    'nickname': String(client['client_nickname']),
    'muted': {
      'input': Boolean(client['input_muted']),
      'output': Boolean(client['output_muted']),
    }
  };
};

Client.prototype.listChannels = async function() {
  let url = this.endpoint + '/clients';

  let request = new Request(url, {
    method: 'GET'
  });

  let response = await fetch(request);
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
        'clients': Array(),
      });
    }

    channel.clients.push({
      'id': Number(client['client_id']),
      'nickname': String(client['client_nickname']),
      'muted': {
        'input': Boolean(client['input_muted']),
        'output': Boolean(client['output_muted']),
      }
    });
  }

  return channels;
};

Client.prototype.muteClient = async function(id, device, mute) {
  let url = this.endpoint + '/clients/' + (mute ? 'mute' : 'unmute');
  let body = JSON.stringify({
    'client_id': Number(id),
    'device': String(device),
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await fetch(request);
  if (!response.ok) {
    throw new Error(`failed to mute/unmute ${device} device of client ${id}`);
  }
};

Client.prototype.moveCursor = async function(channel) {
  let url = this.endpoint + '/cursor/move';
  let body = JSON.stringify({
    'channel_id': Number(channel.id)
  });

  let request = new Request(url, {
    method: 'POST', body
  });

  let response = await fetch(request);
  if (!response.ok) {
    throw new Error(`failed to move cursor to channel ${channel.id}`);
  }
};

Client.prototype.joinCursor = async function() {
  let url = this.endpoint + '/cursor/join';

  let request = new Request(url, {
    method: 'POST'
  });

  let response = await fetch(request);
  if (!response.ok) {
    throw new Error('failed to join channel the cursor points to');
  }
};

Client.prototype.listenEvents = function*() {
  let url = this.endpoint + '/events';
  let source = new EventSource(url);

  let running = true, resolve = () => { };

  source.onerror = function() {
    running = false;
    // TODO reject current promise
  };

  source.onmessage = function(event) {
    let data = JSON.parse(event.data);
    resolve(data);
  };

  while (running) {
    yield new Promise(r => resolve = r);
  }
};
