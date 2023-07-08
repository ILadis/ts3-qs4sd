import os
import subprocess

import decky_plugin

class Plugin:

  # executed in a task when plugin is loaded
  async def _main(self):
    decky_plugin.logger.info('Starting TeamSpeak 3')

    env = dict(os.environ)
    env['XDG_RUNTIME_DIR'] = '/var/run/user/1000'
    env['DISPLAY'] = ':0'

    self.process = subprocess.Popen(['flatpak', 'run', 'com.teamspeak.TeamSpeak'], env=env)

  # TODO consider function to install teamspeak / check for flatpak installation
  # TODO kill process on _unload