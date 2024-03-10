
#include "sdinput.h"
#include "log.h"

struct SDInput* SDInput_getInstance() {
  static struct SDInput input = {0};
  return &input;
}

static int SDInput_openDeviceIfMatches(
    const char *path,
    const struct hidraw_devinfo *device,
    unsigned int ifacenum)
{
  int fd = open(path, O_RDWR|O_NONBLOCK);
  if (fd <= 0) {
    goto error;
  }

  struct hidraw_devinfo dev = {0};
  ioctl(fd, HIDIOCGRAWINFO, &dev);

  if (dev.vendor != device->vendor || dev.product != device->product) {
    Logger_debugLog("Device %s does not match Steam Deck input vid/pid (is %hd/%hd)", path, dev.vendor, dev.product);
    goto error;
  }

  char rawphys[256] = {0};
  ioctl(fd, HIDIOCGRAWPHYS(256), rawphys);

  char ifacename[256] = {0};
  snprintf(ifacename, sizeof(ifacename), "input%u", ifacenum);

  int offset = strlen(rawphys) - strlen(ifacename);
  if (offset < 0  || strcmp(&rawphys[offset], ifacename) != 0) {
    Logger_debugLog("Device %s does not match Steam Deck input physical location (is %s)", path, rawphys);
    goto error;
  }

  return fd;

error:
  close(fd);
  return -1;
}

bool SDInput_tryOpenDevice(struct SDInput *input) {
  const struct hidraw_devinfo device = {
    .vendor = 0x28DE,
    .product = 0x1205,
  };

  // don't try to reopen device
  if (input->fd > 0) {
    return true;
  }

  char path[16] = {0};
  for (int index = 0; index < 255; index++) {
    snprintf(path, sizeof(path), "/dev/hidraw%d", index);

    int fd = SDInput_openDeviceIfMatches(path, &device, 2);
    if (fd > 0) {
      input->fd = fd;
      return true;
    }
  }

  return false;
}

void SDInput_closeDevice(struct SDInput *input) {
  int fd = input->fd;
  if (fd > 0) {
    close(fd);
  }

  struct SDButtons buttons = {0};

  input->fd = 0;
  input->current = buttons;
  input->previous = buttons;
}

bool SDInput_pollState(struct SDInput *input) {
  int fd = input->fd;
  if (fd <= 0) {
    return false;
  }

  unsigned char data[64] = {0};
  const int timeout = 500;

  struct pollfd pfd = { .fd = fd, .events = POLLIN };
  int result = poll(&pfd, 1, timeout);
  if (result <= 0) {
    return false;
  }

  int length = read(fd, data, sizeof(data));
  if (length != sizeof(data)) {
    return false;
  }

  struct SDButtons buttons = {0};

  buttons.keys[SDINPUT_KEY_A]  = (data[ 8] & 0b10000000) > 0;
  buttons.keys[SDINPUT_KEY_X]  = (data[ 8] & 0b01000000) > 0;
  buttons.keys[SDINPUT_KEY_B]  = (data[ 8] & 0b00100000) > 0;
  buttons.keys[SDINPUT_KEY_Y]  = (data[ 8] & 0b00010000) > 0;
  buttons.keys[SDINPUT_KEY_L1] = (data[ 8] & 0b00001000) > 0;
  buttons.keys[SDINPUT_KEY_R1] = (data[ 8] & 0b00000100) > 0;
  buttons.keys[SDINPUT_KEY_L2] = (data[ 8] & 0b00000010) > 0;
  buttons.keys[SDINPUT_KEY_R2] = (data[ 8] & 0b00000001) > 0;
  buttons.keys[SDINPUT_KEY_L3] = (data[10] & 0b01000000) > 0;
  buttons.keys[SDINPUT_KEY_R3] = (data[10] & 0b00000100) > 0;
  buttons.keys[SDINPUT_KEY_L4] = (data[13] & 0b00000010) > 0;
  buttons.keys[SDINPUT_KEY_R4] = (data[13] & 0b00000100) > 0;
  buttons.keys[SDINPUT_KEY_L5] = (data[ 9] & 0b10000000) > 0;
  buttons.keys[SDINPUT_KEY_R5] = (data[10] & 0b00000001) > 0;

  input->previous = input->current;
  input->current = buttons;

  sdinput_onUpdate(input);

  return true;
}

bool SDInput_isKeyHeld(
    struct SDInput *input,
    enum SDInputKey key)
{
  return input->previous.keys[key] == true && input->current.keys[key] == true;
}

bool SDInput_isKeyReleased(
    struct SDInput *input,
    enum SDInputKey key)
{
  return input->previous.keys[key] == false && input->current.keys[key] == false;
}

bool SDInput_hasKeyChanged(
    struct SDInput *input,
    enum SDInputKey key)
{
  return input->previous.keys[key] != input->current.keys[key];
}

bool SDInputKey_byId(
    enum SDInputKey *key,
    int id)
{
  if (id >= 0 && id < SDINPUT_KEY_COUNT) {
    *key = (enum SDInputKey) id;
    return true;
  }

  return false;
}

const char* SDInputKey_getName(enum SDInputKey key) {
  static const char *names[] = {
    [SDINPUT_KEY_A]  = "A",
    [SDINPUT_KEY_B]  = "B",
    [SDINPUT_KEY_X]  = "X",
    [SDINPUT_KEY_Y]  = "Y",
    [SDINPUT_KEY_L1] = "L1",
    [SDINPUT_KEY_L2] = "L2",
    [SDINPUT_KEY_L3] = "L3",
    [SDINPUT_KEY_L4] = "L4",
    [SDINPUT_KEY_L5] = "L5",
    [SDINPUT_KEY_R1] = "R1",
    [SDINPUT_KEY_R2] = "R2",
    [SDINPUT_KEY_R3] = "R3",
    [SDINPUT_KEY_R4] = "R4",
    [SDINPUT_KEY_R5] = "R5",
  };

  return names[key];
}
