import os
import shutil
import subprocess

import decky_plugin

# Note: all plugin arguments are pointing to Plugin and not to an instance of it
class Plugin:

  async def _main(plugin):
    plugin.teamspeak = TeamSpeak()

  async def start(plugin):
    decky_plugin.logger.info('Attempt to starting TeamSpeak 3')

    result = plugin.teamspeak.install_plugin()
    if result is False:
      decky_plugin.logger.error('Could not install TeamSpeak 3 client plugin, maybe TeamSpeak 3 is already running')

    plugin.teamspeak.start()

  async def stop(plugin):
    decky_plugin.logger.info('Attempt to stop TeamSpeak 3')
    plugin.teamspeak.stop()

  async def _unload(plugin):
    decky_plugin.logger.info('Unloading plugin, attempt to stop TeamSpeak 3')
    plugin.teamspeak.stop()

  # TODO consider function to install teamspeak / check for flatpak installation

class TeamSpeak:

  def start(self):
    if self.is_running():
      return

    env = dict(os.environ)
    env['DISPLAY'] = ':0'

    try:
      subprocess.Popen(['flatpak', 'run', 'com.teamspeak.TeamSpeak'], env=env)
    except:
      pass

  def stop(self):
    if not self.is_running():
      return

    try:
      subprocess.run(['flatpak', 'kill', 'com.teamspeak.TeamSpeak'])
    except:
      pass

  def is_running(self):
    try:
      result = subprocess.check_output(['flatpak', 'ps'], encoding='utf-8')
      return 'com.teamspeak.TeamSpeak' in result
    except:
      return False

  def install_plugin(self):
    plugindir = os.getenv('DECKY_PLUGIN_DIR')
    homedir = os.getenv('HOME')

    # can not install plugin while teamspeak is running
    if self.is_running():
      return False

    filename = 'ts3-qs4sd.so'
    srcfile = plugindir + '/bin/' + filename
    dstfile = homedir + '/.var/app/com.teamspeak.TeamSpeak/.ts3client/plugins/' + filename

    if os.path.exists(dstfile):
      os.chmod(dstfile, 0o755)

    try:
      shutil.copy(srcfile, dstfile)
      return True
    except:
      return False
