#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>

#include <sys/ioctl.h>
#include <sys/poll.h>

#include <linux/hidraw.h>

enum SDInputKey {
  SDINPUT_KEY_A,
  SDINPUT_KEY_B,
  SDINPUT_KEY_X,
  SDINPUT_KEY_Y,
  SDINPUT_KEY_L1,
  SDINPUT_KEY_L2,
  SDINPUT_KEY_L3,
  SDINPUT_KEY_L4,
  SDINPUT_KEY_L5,
  SDINPUT_KEY_R1,
  SDINPUT_KEY_R2,
  SDINPUT_KEY_R3,
  SDINPUT_KEY_R4,
  SDINPUT_KEY_R5,
};

#define SDINPUT_KEY_COUNT 14

struct SDInput {
  int fd;
  struct SDButtons {
    bool keys[SDINPUT_KEY_COUNT];
  } current, previous;
};

struct SDInput* SDInput_getInstance();

bool SDInput_tryOpenDevice(struct SDInput *input);
void SDInput_closeDevice(struct SDInput *input);

bool SDInput_pollState(struct SDInput *input);

bool SDInput_isKeyHeld(struct SDInput *input, enum SDInputKey key);
bool SDInput_isKeyReleased(struct SDInput *input, enum SDInputKey key);
bool SDInput_hasKeyChanged(struct SDInput *input, enum SDInputKey key);

bool SDInputKey_byId(enum SDInputKey *key, int id);
const char* SDInputKey_getName(enum SDInputKey key);

// external callbacks
extern void sdinput_onUpdate(struct SDInput *input);

#endif
