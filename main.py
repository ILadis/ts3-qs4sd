import os, io
import shutil
import tempfile
import subprocess
import tarfile
import urllib
import ctypes

import decky_plugin

# Note: all plugin arguments are pointing to Plugin and not to an instance of it
class Plugin:
  async def _main(plugin):
    plugin.teamspeak = FlatpakTeamSpeak()

  async def install(plugin):
    decky_plugin.logger.debug('Attempting to install TeamSpeak 3')
    plugin.teamspeak.install()

  async def start(plugin):
    decky_plugin.logger.debug('Attempting to install TeamSpeak 3 client plugin')
    plugin.teamspeak.install_plugin()

    decky_plugin.logger.debug('Attempting to start TeamSpeak 3')
    plugin.teamspeak.start()

  async def status(plugin):
    installed = plugin.teamspeak.is_installed()
    running = plugin.teamspeak.is_running()

    decky_plugin.logger.debug(f'Querying TeamSpeak 3 status: installed={installed}, running={running}')

    return dict(running = running, installed = installed)

  async def stop(plugin):
    decky_plugin.logger.debug('Attempting to stop TeamSpeak 3')
    plugin.teamspeak.stop()

  async def _unload(plugin):
    decky_plugin.logger.debug('Unloading plugin, attempt to stop TeamSpeak 3')
    plugin.teamspeak.stop()

class TeamSpeak:
  def configdir():
    pass

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

  def install_plugin(self):
    if not self.is_installed() or self.is_running():
      return False

    source = decky_plugin.DECKY_PLUGIN_DIR + '/bin/ts3-qs4sd.so'
    target = self.configdir() + '/plugins/ts3-qs4sd.so'

    if os.path.exists(target):
      os.chmod(target, 0o755)

    try:
      shutil.copy(source, target)
      return True
    except:
      return False

  def add_bookmark(self, dbpath, nickname, bookmark_name, address, port):
    library = decky_plugin.DECKY_PLUGIN_DIR + '/bin/ts3-qs4sd.so'
    handle = ctypes.cdll.LoadLibrary(library)

    add = handle.TS3BookmarkManager_addBookmark
    add.restype = ctypes.c_bool
    add.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint]

    return add(dbpath, nickname, bookmark_name, address, port)

class NativeTeamSpeak(TeamSpeak):
  def __init__(self):
    self.process = None
    self.version = '3.6.2'
    self.installdir = '/opt/teamspeak3'
    self.executable = 'ts3client_linux_amd64'

  def configdir(self):
    homedir = decky_plugin.DECKY_USER_HOME
    return f'{homedir}/.ts3client'

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

    env = dict(os.environ)
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
      return True
    except:
      return False

class FlatpakTeamSpeak(TeamSpeak):
  def configdir(self):
    homedir = decky_plugin.DECKY_USER_HOME
    appsdir = '.var/app/com.teamspeak.TeamSpeak3'
    return f'{homedir}/{appsdir}/.ts3client'

  def is_running(self):
    try:
      ps = subprocess.check_output(['flatpak', 'ps'], encoding='utf-8')
      return 'com.teamspeak.TeamSpeak3' in ps
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
      subprocess.Popen(['flatpak', 'run', 'com.teamspeak.TeamSpeak3'], env=env)
    except:
      pass

  def stop(self):
    if not self.is_running():
      return

    try:
      subprocess.run(['flatpak', 'kill', 'com.teamspeak.TeamSpeak3'])
    except:
      pass

  def is_installed(self):
    try:
      ls = subprocess.check_output(['flatpak', 'list'], encoding='utf-8')
      return 'com.teamspeak.TeamSpeak3' in ls
    except:
      return False

  def install(self):
    if self.is_installed():
      return True

    if self.is_running():
      return False

    try:
      subprocess.Popen(['flatpak', 'install', 'com.teamspeak.TeamSpeak3', '--noninteractive'])
      return True
    except:
      return False
