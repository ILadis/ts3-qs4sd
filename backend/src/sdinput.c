
#include "sdinput.h"
#include "log.h"

struct SDInput* SDInput_getInstance() {
  static struct SDInput input = {
    .verbose = false,
    .device = {
      .vendor = 0x28DE,
      .product = 0x1205,
      .ifacenum = 2,
    }
  };

  return &input;
}

bool SDInput_useDevice(
    struct SDInput *input,
    short vendor, short product,
    unsigned int ifacenum)
{
  // can not change device when it's already opened
  if (input->fd > 0) {
    return false;
  }

  input->device.vendor = vendor;
  input->device.product = product;
  input->device.ifacenum = ifacenum;

  // enables verbose buffer logging
  input->verbose = true;

  return true;
}

static int SDInput_openDeviceIfMatches(
    const char *path,
    const struct hidraw_devinfo *device,
    unsigned int ifacenum)
{
  int fd = open(path, O_RDWR|O_NONBLOCK);
  if (fd <= 0) {
    Logger_debugLog("Failed to open device file: %s", path);
    goto error;
  }

  struct hidraw_devinfo dev = {0};
  ioctl(fd, HIDIOCGRAWINFO, &dev);

  if (dev.vendor != device->vendor || dev.product != device->product) {
    Logger_debugLog("Device file %s does not match input vid/pid (%04hX/%04hX)", path, dev.vendor, dev.product);
    goto error;
  }

  char rawphys[256] = {0};
  ioctl(fd, HIDIOCGRAWPHYS(256), rawphys);

  char ifacename[256] = {0};
  snprintf(ifacename, sizeof(ifacename), "input%u", ifacenum);

  int offset = strlen(rawphys) - strlen(ifacename);
  if (offset < 0  || strcmp(&rawphys[offset], ifacename) != 0) {
    Logger_debugLog("Device file %s does not match input physical location (%s)", path, rawphys);
    goto error;
  }

  Logger_debugLog("Opened device file %s with matching input vid/pid (%04hX/%04hX) and physical location (%s) for polling", path, dev.vendor, dev.product, rawphys);
  return fd;

error:
  close(fd);
  return -1;
}

bool SDInput_tryOpenDevice(struct SDInput *input) {
  struct hidraw_devinfo device = {
    .vendor = input->device.vendor,
    .product = input->device.product,
  };

  // don't try to reopen device
  if (input->fd > 0) {
    return true;
  }

  char path[16] = {0};
  for (int index = 0; index < 255; index++) {
    snprintf(path, sizeof(path), "/dev/hidraw%d", index);

    int fd = SDInput_openDeviceIfMatches(path, &device, input->device.ifacenum);
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

static void SDInput_logBuffer(
    struct SDInput *input,
    unsigned char *data, int length,
    bool success)
{
  if (!input->verbose) {
    return;
  }

  static int previous = -1;
  int current = 0;
  for (int i = 0; i < length; i++) {
    current ^= data[i];
  }

  if (previous == current) {
    return;
  }

  char dump[55] = {0};
  Logger_debugLog("%s", success ? "Device file was polled successful" : "Failed to poll from device file");

  for (int i = 0; i < length; i += 16) {
    char *buffer = dump;
    buffer += sprintf(buffer, "%04X  ", i);

    for (int j = 0; j < 16; j++) {
      if (i + j < length) {
        buffer += sprintf(buffer, "%02X ", data[i + j]);
      }
      if (j == 7) {
        buffer += sprintf(buffer, " ");
      }
    }

    Logger_debugLog("%s", dump);
  }

  previous = current;
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
    goto error;
  }

  int length = read(fd, data, sizeof(data));
  if (length != sizeof(data)) {
    goto error;
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

  SDInput_logBuffer(input, data, sizeof(data), true);
  return true;

error:
  SDInput_logBuffer(input, data, sizeof(data), false);
  return false;
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
