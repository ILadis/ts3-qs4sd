
#include "sdinput.h"

struct SDInput* SDInput_getInstance() {
  static struct SDInput input = {0};
  return &input;
}

bool SDInput_tryOpenDevice(struct SDInput *input) {
  const unsigned short vid = 0x28DE, pid = 0x1205, usage = 0xFFFF;

  // don't try to reopen device
  if (input->device != NULL) {
    return true;
  }

  struct hid_device_info *devices = hid_enumerate(vid, pid); // find all devices
  struct hid_device_info *info = devices;

  hid_device *device = NULL;
  while (info) {
    if (info->usage_page == usage) {
      device = hid_open_path(info->path);
      break;
    }
    info = info->next;
  }

  if (device == NULL) {
    return false;
  }

  input->device = device;

  return true;
}

void SDInput_closeDevice(struct SDInput *input) {
  hid_device *device = input->device;
  if (device != NULL) {
    hid_close(device);
  }

  struct SDButtons buttons = {0};

  input->device = NULL;
  input->current = buttons;
  input->previous = buttons;
}

bool SDInput_pollState(struct SDInput *input) {
  hid_device *device = input->device;
  if (device == NULL) {
    return false;
  }

  unsigned char data[64] = {0};
  const int timeout = 500;

  int length = hid_read_timeout(device, data, sizeof(data), timeout);
  if (length != sizeof(data)) {
    return false;
  }

  struct SDButtons buttons = {0};

  buttons.A  = data[ 8] & 0b10000000;
  buttons.X  = data[ 8] & 0b01000000;
  buttons.B  = data[ 8] & 0b00100000;
  buttons.Y  = data[ 8] & 0b00010000;
  buttons.L1 = data[ 8] & 0b00001000;
  buttons.R1 = data[ 8] & 0b00000100;
  buttons.L2 = data[ 8] & 0b00000010;
  buttons.R2 = data[ 8] & 0b00000001;

  buttons.L5  = data[ 9] & 0b10000000;
  buttons.R5  = data[10] & 0b00000001;
  buttons.R3  = data[10] & 0b00000100;

  input->previous = input->current;
  input->current = buttons;

  sdinput_onUpdate(input);

  return true;
}
