
#include "paudio.h"
#include "sdinput.h"

#include "log.h"

void PAudio_task() {
  static bool once = true;
  struct PAudio *paudio = PAudio_getInstance();
  if (once && !PAudio_connect(paudio)) {
    Logger_errorLog("Could not connect to Pulse Audio server");
    once = false;
  }

  PAudio_runLoop(paudio);
}

void SDInput_task() {
  static bool once = true;
  struct SDInput *input = SDInput_getInstance();
  if (once && !SDInput_tryOpenDevice(input)) {
    Logger_errorLog("Could not open Steam Deck input device");
    once = false;
  }

  SDInput_pollState(input);
}
