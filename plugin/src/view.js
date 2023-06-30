
import { createElement as $ } from 'react';

import {
  Field,
  Focusable,
  DialogButton,
  SliderField,
  PanelSection,
  PanelSectionRow,
} from 'decky-frontend-lib';

export function QuickAccessPanel(props) {
  return (
    props.content == 'bookmarks' ? $(TS3BookmarkList, props) :
    props.content == 'dashboard' ? $(TS3Dashboard, props) : $('div')
  );
}

export function TS3BookmarkList({ bookmarks, bookmarkSelected }) {
  return (
    $(PanelSection, { title: 'SERVERS' },
      $(PanelSectionRow, null, bookmarks.map(bookmark =>
        $(Field, { onClick: () => bookmarkSelected(bookmark), label: bookmark.name })),
      )
    )
  );
}

export function TS3Dashboard({ server, self, channels, outputs, toggleOutputs, toggleMute, outputChanged, disconnectClicked }) {
  return (
    $(PanelSection, { title: server.name },
      $(PanelSectionRow, null, $(TS3DashboardActions, { self, toggleMute, disconnectClicked })),

      $(PanelSectionRow, null,
        $(Field, { childrenLayout: 'below', bottomSeparator: 'standard' },
          $(DialogButton, { onClick: () => toggleOutputs() }, 'Volume Settings')
        )
      ),

      outputs.map(output => $(PanelSectionRow, null, $(TS3OutputDevice, { output, outputChanged }))),
      channels.map(channel => $(PanelSectionRow, null, $(TS3ClientList, { channel }))),
    )
  );
}

export function TS3DashboardActions({ self, toggleMute, disconnectClicked }) {
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
        $(DialogButton, { onClick: () => disconnectClicked() }, 'Disconnect'),
      )
    )
  );
}

export function TS3OutputDevice({ output, outputChanged }) {
  return (
    $(SliderField, {
      label: output.name,
      value: output.volume,
      onChange: (value) => outputChanged(output, value),
      min: 0,
      max: 1,
      step: 0.05
    })
  );
}

export function TS3ClientList({ channel }) {
  const style = {
    'display': 'inline-block',
    'maxWidth': '100%',
    'padding': '0',
    'margin': '0',
    'fontSize': '14px',
    'fontWeight': 'normal',
    'lineHeight': '22px',
    'color': '#c5d6d4',
    'whiteSpace': 'nowrap',
    'textOverflow': 'ellipsis',
    'overflow': 'hidden',
  };

  return (
    $(Field, { childrenLayout: 'below' },
      $(Focusable, null, $('h3', { style }, channel.name)),
      $(Focusable, null, channel.clients.map(client =>
        $(TS3ClientItem, { client })
      ))
    )
  );
}

export function TS3ClientItem({ client }) {
  const style = {
    'display': 'block',
    'lineHeight': '15px',
    'whiteSpace': 'nowrap',
    'textOverflow': 'ellipsis',
    'overflow': 'hidden',
  };

  return (
    $(Field, { label: $(TS3ClientAvatar), childrenContainerWidth: 'max', bottomSeparator: 'none' },
      $('span', { style: { ...style, 'fontSize': '15px', 'color': '#b3dfff' } }, client.nickname),
      $('span', { style: { ...style, 'fontSize': '12px', 'color': '#4cb4ff' } }, 'Online'),
    )
  );
}

export function TS3ClientAvatar() {
  const style = {
    'margin': 0,
    'width': '32px',
    'height': '32px',
    'border-right': '2px solid #6dcff6'
  };

  const url = 'https://avatars.cloudflare.steamstatic.com/fef49e7fa7e1997310d705b2a6158ff8dc1cdfeb_medium.jpg';

  return $('figure', { style }, $('img', { src: url, width: '100%', height: '100%' }));
}

export function TS3IconButton({ icon, onClick }) {
  const style = {
    'padding': '7px 10px 4px',
    'width': 'auto',
    'min-width': 0,
    'flex-shrink': 0
  };

  return $(DialogButton, { style, onClick }, icon);
}

export function TS3InputMute({ state }) {
  const paths = {
    [true]:  'M34.3 29.95 32.15 27.8Q33.2 26.5 33.7 24.875Q34.2 23.25 34.2 21.55H37.2Q37.2 23.85 36.45 26Q35.7 28.15 34.3 29.95ZM27.9 23.55 25.25 20.95V9.05Q25.25 8.2 24.65 7.6Q24.05 7 23.2 7Q22.35 7 21.75 7.6Q21.15 8.2 21.15 9.05V16.8L18.15 13.8V9.05Q18.15 6.95 19.625 5.475Q21.1 4 23.2 4Q25.3 4 26.775 5.475Q28.25 6.95 28.25 9.05V21.55Q28.25 21.95 28.175 22.55Q28.1 23.15 27.9 23.55ZM41.35 45.45 29.65 33.75Q28.5 34.3 27.25 34.675Q26 35.05 24.7 35.2V42H21.7V35.2Q16.4 34.65 12.8 30.775Q9.2 26.9 9.2 21.55H12.2Q12.2 26.05 15.425 29.175Q18.65 32.3 23.2 32.3Q24.4 32.3 25.475 32.1Q26.55 31.9 27.65 31.35L1.75 5.5L3.7 3.6L43.45 43.35ZM23.05 18.7Q23.05 18.7 23.05 18.7Q23.05 18.7 23.05 18.7Q23.05 18.7 23.05 18.7Q23.05 18.7 23.05 18.7Z',
    [false]: 'M24 26.85Q21.85 26.85 20.4 25.3Q18.95 23.75 18.95 21.55V9Q18.95 6.9 20.425 5.45Q21.9 4 24 4Q26.1 4 27.575 5.45Q29.05 6.9 29.05 9V21.55Q29.05 23.75 27.6 25.3Q26.15 26.85 24 26.85ZM24 15.45Q24 15.45 24 15.45Q24 15.45 24 15.45Q24 15.45 24 15.45Q24 15.45 24 15.45Q24 15.45 24 15.45Q24 15.45 24 15.45Q24 15.45 24 15.45Q24 15.45 24 15.45ZM22.5 42V35.2Q17.2 34.65 13.6 30.75Q10 26.85 10 21.55H13Q13 26.1 16.225 29.2Q19.45 32.3 24 32.3Q28.55 32.3 31.775 29.2Q35 26.1 35 21.55H38Q38 26.85 34.4 30.75Q30.8 34.65 25.5 35.2V42ZM24 23.85Q24.9 23.85 25.475 23.175Q26.05 22.5 26.05 21.55V9Q26.05 8.15 25.45 7.575Q24.85 7 24 7Q23.15 7 22.55 7.575Q21.95 8.15 21.95 9V21.55Q21.95 22.5 22.525 23.175Q23.1 23.85 24 23.85Z'
  };

  return (
    $('svg', { viewBox: '0 0 48 48', width: '24px' },
      $('path', { fill: 'currentColor', 'd': paths[state] })
    )
  );
}

export function TS3OutputMute({ state }) {
  const paths = {
    [true]:  'M40.65 45.2 34.05 38.6Q32.65 39.6 31.025 40.325Q29.4 41.05 27.65 41.45V38.35Q28.8 38 29.875 37.575Q30.95 37.15 31.9 36.45L23.65 28.15V40L13.65 30H5.65V18H13.45L2.45 7L4.6 4.85L42.8 43ZM38.85 33.6 36.7 31.45Q37.7 29.75 38.175 27.85Q38.65 25.95 38.65 23.95Q38.65 18.8 35.65 14.725Q32.65 10.65 27.65 9.55V6.45Q33.85 7.85 37.75 12.725Q41.65 17.6 41.65 23.95Q41.65 26.5 40.95 28.95Q40.25 31.4 38.85 33.6ZM18.6 23.15ZM32.15 26.9 27.65 22.4V15.9Q30 17 31.325 19.2Q32.65 21.4 32.65 24Q32.65 24.75 32.525 25.475Q32.4 26.2 32.15 26.9ZM23.65 18.4 18.45 13.2 23.65 8ZM20.65 32.7V25.2L16.45 21H8.65V27H14.95Z',
    [false]: 'M28 41.45V38.35Q32.85 36.95 35.925 32.975Q39 29 39 23.95Q39 18.9 35.95 14.9Q32.9 10.9 28 9.55V6.45Q34.2 7.85 38.1 12.725Q42 17.6 42 23.95Q42 30.3 38.1 35.175Q34.2 40.05 28 41.45ZM6 30V18H14L24 8V40L14 30ZM27 32.4V15.55Q29.7 16.4 31.35 18.75Q33 21.1 33 24Q33 26.95 31.35 29.25Q29.7 31.55 27 32.4ZM21 15.6 15.35 21H9V27H15.35L21 32.45ZM16.3 24Z'
  };

  return (
    $('svg', { viewBox: '0 0 48 48', width: '24px' },
      $('path', { fill: 'currentColor', 'd': paths[state] })
    )
  );
}

