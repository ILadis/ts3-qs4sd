
import { createElement as $, useEffect, useState } from 'react';

import {
  Field,
  Focusable,
  DialogButton,
  SliderField,
  PanelSection,
  PanelSectionRow,
  ModalRoot,
  DialogHeader,
  DialogBody,
  showModal,
} from 'decky-frontend-lib';

import * as Icons from './icons';
import styles from './styles';

export function TS3QuickAccessPanel(props) {
  const content = ({
    'setup':     TS3SetupHints,
    'bookmarks': TS3BookmarkList,
    'dashboard': TS3Dashboard,
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

export function TS3ChannelBrowserDialog(props) {
  const {
    browseChannels,
    joinChannel,
    closeModal,
  } = props;

  const [channels, setChannels] = useState([]);
  const [parent, setParent] = useState([]);

  const channel = { id: 0 };
  useEffect(() => viewChannels(channel), [])

  async function viewChannels(channel) {
    const channels = await browseChannels(channel);
    if (channels.length > 0) {
      setParent([channel, ...parent]);
      setChannels(channels);
    }
  }

  async function parentChannel() {
    const channel = (parent.shift(), parent.shift());
    if (!channel) {
      closeModal();
    } else {
      viewChannels(channel);
    }
  }

  const style = {
    'display': 'flex',
    'gap': '4px'
  };

  return (
    $(ModalRoot, { closeModal: () => parentChannel() },
      $(DialogHeader, null, 'Channel Browser'),
      $(DialogBody, null, channels.map(channel =>
        $(PanelSectionRow, { key: channel.id },
          $(Field, { label: channel.name, childrenLayout: 'inline', inlineWrap: 'keep-inline' },
            $(Focusable, { style },
              $(DialogButton, { onClick: () => viewChannels(channel), style: { 'min-width': 0 } }, 'View'),
              $(DialogButton, { onClick: () => (joinChannel(channel), closeModal()), style: { 'min-width': 0 } }, 'Join')
            )
          )
        )
      ))
    )
  );
}

export function TS3Dashboard(props) {
  const {
    server,
    channels,
    outputs,
    toggleOutputs,
    outputChanged,
    joinChannel,
  } = props;

  return (
    $(PanelSection, { title: server.name },
      $(PanelSectionRow, null, $(TS3DashboardActions, props)),

      $(PanelSectionRow, null, $(TS3DashboardAction, { onClick: () => showModal($(TS3ChannelBrowserDialog, props)), label: 'View Channels' })),
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

export function TS3LogoIcon() {
  const path = Icons.TS3Logo;
  return (
    $('svg', { viewBox: '0 0 460 460', width: '20px' },
      $('path', { fill: 'currentColor', 'd': path })
    )
  );
}
