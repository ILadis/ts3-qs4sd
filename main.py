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
      decky_plugin.logger.error('Could not install TeamSpeak 3 client plugin')

    plugin.teamspeak.start()

    running = plugin.teamspeak.is_running(1)
    pid = plugin.teamspeak.get_processid()

    if running:
      decky_plugin.logger.info('TeamSpeak running with pid: ' + str(pid))
    else:
      decky_plugin.logger.info('TeamSpeak not started')

    return dict(running = running)

  async def _unload(plugin):
    decky_plugin.logger.info('Unloading plugin, attempt to stop TeamSpeak 3')

    plugin.teamspeak.stop()

    running = plugin.teamspeak.is_running(1)

    if running:
      decky_plugin.logger.info('TeamSpeak stopped')
    else:
      decky_plugin.logger.info('TeamSpeak not stopped')

  # TODO consider function to install teamspeak / check for flatpak installation

class TeamSpeak:

  def __init__(self):
    self.process = None

  def start(self):
    if self.is_running():
      return

    env = dict(os.environ)
    env['DISPLAY'] = ':0'

    try:
      self.process = subprocess.Popen(['flatpak', 'run', 'com.teamspeak.TeamSpeak'], env=env, user='deck')
    except:
      pass

  def stop(self):
    try:
      subprocess.Popen(['flatpak', 'kill', 'com.teamspeak.TeamSpeak'], user='deck')
    except:
      pass

    if self.process is not None:
      self.process.wait()

  def is_running(self, timeout = None):
    if self.process is None:
      return False

    if self.process.returncode is not None:
      return False

    try:
      if timeout is not None:
        self.process.wait(timeout)
      else:
        self.process.poll()
    except:
      pass

    return self.process.returncode is None

  def get_processid(self):
    if self.process is None:
      return None
    else:
      return self.process.pid

  def install_plugin(self):
    plugindir = os.getenv('DECKY_PLUGIN_DIR')
    homedir = os.getenv('HOME')

    filename = 'ts3-qs4sd.so'
    srcfile = plugindir + '/bin/' + filename
    dstfile = homedir + '/.var/app/com.teamspeak.TeamSpeak/.ts3client/plugins/' + filename

    if os.path.exists(dstfile):
      os.chmod(dstfile, 0o755)

    try:
      shutil.copy(srcfile, dstfile)
      return True
    except e:
      return False
