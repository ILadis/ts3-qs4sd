
import { createElement as $, useEffect, useState } from 'react';

import {
  Tabs,
  Field,
  Focusable,
  DialogButton,
  SliderField,
  PanelSection,
  PanelSectionRow,
} from 'decky-frontend-lib';

import * as Icons from './icons';
import styles from './styles';

export function TS3QuickAccessPanel(props) {
  const content = ({
    'setup':     TS3SetupHints,
    'bookmarks': TS3BookmarkList,
    'dashboard': TS3Tabs,
  })[props.content];

  return (
    $(Focusable, null,
      $('style', null, styles),
      $(content || 'div', props)
    )
  );
}

export function TS3SetupHints() {
  return (
    $(PanelSection, null,
      $(PanelSectionRow, null,
        $(Field, { label: 'SETUP' }, ''
          + 'TeamSpeak 3 is not installed. Please switch to gaming mode and '
          + 'download TeamSpeak 3 from the Discover store. Make sure to add '
          + 'all TeamSpeak servers you want to connect to as bookmarks. '
          + 'Then come back here.'
        ),
      )
    )
  );
}

export function TS3BookmarkList(props) {
  const {
    bookmarks,
    connectTo,
  } = props;

  return (
    $(PanelSection, { title: 'SERVERS' },
      $(PanelSectionRow, null, bookmarks.map(bookmark =>
        $(Field, { onClick: () => connectTo(bookmark), label: bookmark.name })),
      )
    )
  );
}

export function TS3Tabs(props) {
  const { server } = props;
  const [currentTab, setCurrentTab] = useState('dashboard');

  const tabs = [
    { title: server.name, content: $(TS3Dashboard, props), id: 'dashboard' },
    { title: 'Channel Browser', content: $(TS3ChannelBrowser, props), id: 'browser' },
    { title: 'Settings', content: $('div'), id: 'settings' },
  ];

  return (
    $('div', { className: 'dashboard-tabs' },
      $(Tabs, { tabs, activeTab: currentTab, onShowTab: (id) => setCurrentTab(id) })
    )
  );
}

export function TS3ChannelBrowser(props) {
  const {
    browseChannels,
    joinChannel,
  } = props;

  // TODO move state handling to app component
  const [channels, setChannels] = useState([]);
  const [parent, setParent] = useState([]);

  if (parent.length == 0 && channels.length == 0) {
    const channel = { id: 0 };
    viewChannels(channel);
  }

  async function viewChannels(channel) {
    const channels = await browseChannels(channel);
    if (channels.length > 0) {
      setParent([channel, ...parent]);
      setChannels(channels);
    }
  }

  function parentChannel() {
    const channel = (parent.shift(), parent.shift());
    if (channel) {
      viewChannels(channel);
    }
  }

  return (
    $(PanelSection, null, channels.map(channel =>
      $(PanelSectionRow, { key: channel.id }, channel.maxClients <= 0
        ? $(Field, { label: channel.name, focusable: true, highlightOnFocus: true, onOKButton: () => viewChannels(channel), onCancelButton: () => parentChannel(), icon: $(TS3ExpandMore) }, null)
        : $(Field, { label: channel.name, focusable: true, highlightOnFocus: true, onOKButton: () => joinChannel(channel), onCancelButton: () => parentChannel() }, null)
      )
    ))
  );
}

export function TS3Dashboard(props) {
  const {
    channels,
    outputs,
    toggleOutputs,
    outputChanged,
    joinChannel,
  } = props;

  return (
    $(PanelSection, null,
      $(PanelSectionRow, null, $(TS3DashboardActions, props)),
      $(PanelSectionRow, null, $(TS3DashboardAction, { onClick: () => toggleOutputs(), label: 'Volume Settings' })),

      outputs.map(output => $(PanelSectionRow, null, $(TS3OutputDevice, { output, outputChanged }))),
      channels.map(channel => $(PanelSectionRow, null, $(TS3ClientList, { channel, joinChannel }))),
    )
  );
}

export function TS3DashboardAction(props) {
  const {
    label,
    onClick,
  } = props;

  return (
    $(Field, { childrenLayout: 'below', bottomSeparator: 'standard' },
      $(DialogButton, { onClick }, label)
    )
  );
}

export function TS3DashboardActions(props) {
  const {
    self,
    toggleMute,
    disconnect,
  } = props;

  const inputMuted  = self.muted['input'];
  const outputMuted = self.muted['output'];

  const style = {
    'display': 'flex',
    'gap': '4px'
  };

  return (
    $(Field, { childrenLayout: 'below', bottomSeparator: 'standard' },
      $(Focusable, { style },
        $(TS3IconButton, { onClick: () => toggleMute('input'), icon: $(TS3InputMute, { state: inputMuted }) }),
        $(TS3IconButton, { onClick: () => toggleMute('output'), icon: $(TS3OutputMute, { state: outputMuted }) }),
        $(DialogButton, { onClick: () => disconnect() }, 'Disconnect'),
      )
    )
  );
}

export function TS3OutputDevice(props) {
  const {
    output,
    outputChanged,
  } = props;

  return (
    $(SliderField, {
      label: output.name,
      value: output.volume,
      onChange: (value) => outputChanged(output, value),
      min: 0, max: 1,
      step: 0.05
    })
  );
}

export function TS3ClientList(props) {
  const {
    channel,
    joinChannel,
  } = props;

  return (
    $(Field, { childrenLayout: 'below', onClick: () => joinChannel(channel) },
      $(Focusable, null, $('h3', { className: 'channel-headline' },
        $('span', { className: 'channel-count' }, channel.clients.length), channel.name
      )),
      $(Focusable, null, channel.clients.map(client =>
        $(TS3ClientItem, { client })
      ))
    )
  );
}

export function TS3ClientItem({ client }) {
  return (
    $(Field, { label: $(TS3ClientAvatar, { client, key: client.id }), className: 'client-item-field', childrenContainerWidth: 'max', bottomSeparator: 'none' },
      $('span', { className: 'client-item nickname' }, client.nickname),
      $('span', { className: 'client-item status' }, 'Online'),
    )
  );
}

export function TS3ClientAvatar({ client }) {
  const fallback = 'https://avatars.cloudflare.steamstatic.com/fef49e7fa7e1997310d705b2a6158ff8dc1cdfeb_medium.jpg';
  const [url, setUrl] = useState(client.avatar);

  const onError = url == fallback ? null : () => setUrl(fallback);

  return (
    $('figure', { className: 'client-avatar' },
      $('img', { src: url, width: '100%', height: '100%', onError })
    )
  );
}

export function TS3IconButton({ icon, onClick }) {
  return $(DialogButton, { className: 'icon-button', onClick }, icon);
}

export function TS3InputMute({ state }) {
  const paths = {
    [true]:  Icons.InputMuteOn,
    [false]: Icons.InputMuteOff,
  };

  return (
    $('svg', { viewBox: '0 0 48 48', width: '24px' },
      $('path', { fill: 'currentColor', 'd': paths[state] || '' })
    )
  );
}

export function TS3OutputMute({ state }) {
  const paths = {
    [true]:  Icons.OutputMuteOn,
    [false]: Icons.OutputMuteOff,
  };

  return (
    $('svg', { viewBox: '0 0 48 48', width: '24px' },
      $('path', { fill: 'currentColor', 'd': paths[state] || '' })
    )
  );
}

export function TS3ExpandMore() {
  const path = Icons.ExpandMore;
  return (
    $('svg', { viewBox: '0 0 36 36', width: '16px' },
      $('path', { fill: 'currentColor', 'd': path })
    )
  );
}

export function TS3LogoIcon() {
  const path = Icons.TS3Logo;
  return (
    $('svg', { viewBox: '0 0 460 460', width: '20px' },
      $('path', { fill: 'currentColor', 'd': path })
    )
  );
}
