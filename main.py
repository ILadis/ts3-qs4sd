import os
import subprocess

import decky_plugin

# Note: all plugin arguments are pointing to Plugin and not to an instance of it
class Plugin:

  async def _main(plugin):
    plugin.teamspeak = TeamSpeak()

  async def start(plugin):
    decky_plugin.logger.info('Attempt to starting TeamSpeak 3')

    plugin.teamspeak.start()

    running = plugin.teamspeak.is_running(1)
    pid = plugin.teamspeak.getpid()

    if running:
      decky_plugin.logger.info('TeamSpeak running with pid: ' + str(pid))
    else:
      decky_plugin.logger.info('TeamSpeak not started')

    return dict(running = running)

  async def _unload(plugin):
    decky_plugin.logger.info('Unloading plugin, attempt to stop TeamSpeak 3')

    plugin.teamspeak.stop()

    running = plugin.teamspeak.is_running(1)
    rc = plugin.teamspeak.getrc()

    if running:
      decky_plugin.logger.info('TeamSpeak stopped with rc: ' + str(rc))
    else:
      decky_plugin.logger.info('TeamSpeak not stopped')

  # TODO consider function to install teamspeak / check for flatpak installation

class TeamSpeak:

  def __init__(self):
    self.process = None

  def start(self):
    env = dict(os.environ)
    env['XDG_RUNTIME_DIR'] = '/var/run/user/1000'
    env['DISPLAY'] = ':0'

    if not self.is_running():
      self.process = subprocess.Popen(['flatpak', 'run', 'com.teamspeak.TeamSpeak'], env=env, user='deck')

  def stop(self):
    subprocess.Popen(['flatpak', 'kill', 'com.teamspeak.TeamSpeak'], user='deck')

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

  def getpid(self):
    if self.process is None:
      return None
    else:
      return self.process.pid

  def getrc(self):
    if self.process is None:
      return None
    else:
      return self.process.returncode
