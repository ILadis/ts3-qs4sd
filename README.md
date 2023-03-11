A TeamSpeak 3 client plugin that integrates TeamSpeak 3 into Steam Deck's quick access menu.

<img src="screenshot.jpg" width="50%">
<img src="screenshot2.jpg" width="50%">

## Features

- Connect to bookmarked servers.
- See connected clients and join their channels.
- Mute/Unmute microphone and speakers.
- Control audio volumes of running applications.
- Disconnect from current server.
- Push-to-Talk via shoulder buttons (bound to L5).

**Missing/Planned features**:  
- Display client avatars and their current status.
- Make compatible with [PluginLoader](https://github.com/SteamDeckHomebrew/PluginLoader).
- Add navigation via controller/buttons.
- Add logging for easier trouble shooting.

## Building

The following dependencies are required to build the plugin:
1. [mjson](https://github.com/cesanta/mjson): for JSON parsing
1. [mongoose](https://github.com/cesanta/mongoose): for HTTP server/client and web sockets
1. [incbin](https://github.com/graphitemaster/incbin): for embedding static resources
1. [ts3client-pluginsdk](https://github.com/TeamSpeak-Systems/ts3client-pluginsdk): the TeamSpeak 3 plugin SDK
1. libpulse: to control audio volumes of applications

To download dependencies 1-4 run `make vendor` (this requires `wget`). All other dependencies need to be installed manually. Then run `make` to build the plugin.

## Installing

Follow these steps in order to install and setup the plugin on your Steam Deck:
1. Switch to desktop mode, download TeamSpeak from the Discover store and launch it.
1. Download a [pre build version](https://github.com/ILadis/ts3-qs4sd/releases) of the plugin and copy it into the TeamSpeak plugin folder at: `/home/deck/.var/app/com.teamspeak.TeamSpeak/.ts3client/plugins`
1. In TeamSpeak open Tools ⇾ Options ⇾ Addons and check if the plugin was loaded successfully; try restarting TeamSpeak if it doesn't show up.
1. Add all TeamSpeak servers you want to connect to as bookmarks (Bookmarks ⇾ Manage Bookmarks).
1. If you want to use Push-to-Talk allow TeamSpeak to access your Steam Deck inputs: `flatpak override --user com.teamspeak.TeamSpeak --device=all`
1. Add TeamSpeak as a non-Steam game; then switch back to gaming mode.
1. In gaming mode, open the Steam Deck settings.
1. Navigate to System ⇾ System Settings and toggle "Enable Developer Mode".
1. Then navigate to Developer ⇾ Miscellaneous and enable "CEF Remote Debugging".
1. Launch TeamSpeak from the Home menu, then press the STEAM button and switch back to the Home menu.
1. Open the quick access menu, a TeamSpeak icon should appear at the bottom.
