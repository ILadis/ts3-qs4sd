
export const english = {
  'setup.headline': 'SETUP',
  'setup.instructions': ''
    + 'TeamSpeak 3 is not installed. Please switch to desktop mode and '
    + 'download TeamSpeak 3 from the Discover store. Make sure to add '
    + 'all TeamSpeak servers you want to connect to as bookmarks. '
    + 'Then come back here.',

  'bookmarks.headline': 'SERVERS',
  'browser.headline': 'CHANNEL BROWSER',

  'dashboard.actions.disconnect': 'Disconnect',
  'dashboard.actions.browser': 'Channel Browser',

  'settings.headline': 'SETTINGS',

  'settings.volumes': 'Volumes',
  'settings.volumes.description': ''
    + 'View and change volumes of applications which are '
    + 'playing back audio.',
  'settings.volumes.toggle': (state) => state ? 'Show applications' : 'Hide applications',

  'settings.mic': 'Microphone',
  'settings.mic.description': ''
    + 'Select the active input device for TeamSpeak.',

  'settings.ptt': 'Push to Talk',
  'settings.ptt.rebind': (state) => state ? 'Press a button...' : 'Bind PTT hotkey',
  'settings.ptt.clear': 'Clear PTT hotkey',

  'settings.ptt.state.unavailable': 'Push to Talk is unavailable, additional setup is required.',
  'settings.ptt.state.disabled': 'Push to Talk can be enabled by selecting L/R 4-5 as a hotkey button.',
  'settings.ptt.state.rebinding': 'Press L/R 4-5 now to select a button as a hotkey for Push to Talk.',
  'settings.ptt.state.active': (hotkey) => `Push to Talk is enabled and bound to hotkey button ${hotkey}.`,

  'password.headline': 'Enter Password',
  'password.ok': 'OK',
  'password.cancel': 'Cancel',
};
