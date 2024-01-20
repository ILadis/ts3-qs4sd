import os, io
import shutil
import tempfile
import subprocess
import tarfile
import urllib

import decky_plugin

# Note: all plugin arguments are pointing to Plugin and not to an instance of it
class Plugin:
  async def _main(plugin):
    plugin.teamspeak = NativeTeamSpeak()

  async def install(plugin):
    decky_plugin.logger.debug('Attempt to install TeamSpeak 3')
    plugin.teamspeak.install()

  async def start(plugin):
    decky_plugin.logger.debug('Attempt to starting TeamSpeak 3')
    plugin.teamspeak.start()

  async def status(plugin):
    installed = plugin.teamspeak.is_installed()
    running = plugin.teamspeak.is_running()

    decky_plugin.logger.debug(f'Querying TeamSpeak 3 status: installed={installed}, running={running}')

    return dict(running = running, installed = installed)

  async def stop(plugin):
    decky_plugin.logger.debug('Attempt to stop TeamSpeak 3')
    plugin.teamspeak.stop()

  async def _unload(plugin):
    decky_plugin.logger.debug('Unloading plugin, attempt to stop TeamSpeak 3')
    plugin.teamspeak.stop()

class TeamSpeak:
  @staticmethod
  def homedir():
    homedir = decky_plugin.DECKY_USER_HOME
    appsdir = '.var/app/com.teamspeak.TeamSpeak'

    if os.path.exists(f'{homedir}/{appsdir}'):
      homedir = f'{homedir}/{appsdir}'

    return homedir

  def is_running(self):
    return False

  def start(self):
    pass

  def stop(self):
    pass

  def is_installed(self):
    return False

  def install(self):
    return False

  def install_plugin(self, plugindir, homedir):
    filename = 'ts3-qs4sd.so'
    srcfile = f'{plugindir}/bin/{filename}'
    dstfile = f'{homedir}/.ts3client/plugins/{filename}'

    if os.path.exists(dstfile):
      os.chmod(dstfile, 0o755)

    try:
      shutil.copy(srcfile, dstfile)
      return True
    except:
      return False

class NativeTeamSpeak(TeamSpeak):
  process = None
  version = '3.6.2'
  installdir = '/tmp/teamspeak3'
  executable = 'ts3client_linux_amd64'

  def is_running(self):
    if self.process is None:
      return False

    returncode = self.process.poll()
    running = returncode is None

    return running

  def start(self):
    if self.is_running():
      return

    args = ['-platform', 'xcb']
    executable = f'{self.installdir}/{self.executable}'

    uid = os.getuid()
    homedir = TeamSpeak.homedir()

    env = dict(os.environ)
    env['HOME'] = homedir
    env['DISPLAY'] = ':0'
    env['PULSE_SERVER'] = f'unix:/run/user/{uid}/pulse/native'
    env['PULSE_CLIENTCONFIG'] = f'/run/user/{uid}/pulse/config'
    env['QT_PLUGIN_PATH'] = '.'
    env['LD_LIBRARY_PATH'] = '.'

    try:
      self.process = subprocess.Popen(args, executable=executable, env=env, cwd=self.installdir)
      self.process.poll()
    except:
      pass

  def stop(self):
    if not self.is_running():
      return

    try:
      self.process.kill()
      self.process.wait(timeout=5)
    except:
      pass

  def is_installed(self):
    executable = f'{self.installdir}/{self.executable}'
    installed = os.path.exists(executable) is True

    return installed

  def install(self):
    if self.is_installed():
      return True

    if self.is_running():
      return False

    url = f'http://files.teamspeak-services.com/releases/client/{self.version}/TeamSpeak3-Client-linux_amd64-{self.version}.run'

    try:
      wget = urllib.request.urlopen(url)

      file = tempfile.TemporaryFile()
      shutil.copyfileobj(wget, file)

      os.makedirs(self.installdir, mode=0o700, exist_ok=True)
      file.seek(0x166E6)

      tar = tarfile.open(fileobj=file)
      tar.extractall(path=self.installdir)

      file.close()
    except:
      return False

    plugindir = decky_plugin.DECKY_PLUGIN_DIR
    homedir = TeamSpeak.homedir()

    return self.install_plugin(plugindir, homedir)

class FlatpakTeamSpeak(TeamSpeak):
  def is_running(self):
    try:
      ps = subprocess.check_output(['flatpak', 'ps'], encoding='utf-8')
      return 'com.teamspeak.TeamSpeak' in ps
    except:
      return False

  def start(self):
    if self.is_running():
      return

    uid = os.getuid()

    env = dict(os.environ)
    env['DISPLAY'] = ':0'
    env['PULSE_SERVER'] = f'unix:/run/user/{uid}/pulse/native'
    env['PULSE_CLIENTCONFIG'] = f'/run/user/{uid}/pulse/config'

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

  def is_installed(self):
    try:
      ls = subprocess.check_output(['flatpak', 'list'], encoding='utf-8')
      return 'com.teamspeak.TeamSpeak' in ls
    except:
      return False

  def install(self):
    if self.is_installed():
      return True

    if self.is_running():
      return False

    try:
      subprocess.Popen(['flatpak', 'install', 'com.teamspeak.TeamSpeak3', '--noninteractive'])
    except:
      return False

    plugindir = decky_plugin.DECKY_PLUGIN_DIR
    homedir = TeamSpeak.homedir()

    return self.install_plugin(plugindir, homedir)
