
#include "paudio.h"
#include "sdinput.h"

#include "log.h"

bool PAudio_task() {
  static bool once = true;
  struct PAudio *paudio = PAudio_getInstance();

  if (once && !PAudio_connect(paudio)) {
    Logger_errorLog("Could not connect to Pulse Audio server");
    return false;
  }

  if (once) {
    Logger_debugLog("Connected to Pulse Audio server");
    once = false;
  }

  if (!PAudio_runLoop(paudio)) {
    Logger_infoLog("Running Pulse Audio main loop failed, terminating task");
    return false;
  }

  return true;
}

bool SDInput_task() {
  static bool once = true;
  struct SDInput *input = SDInput_getInstance();

  if (once && !SDInput_tryOpenDevice(input)) {
    Logger_errorLog("Could not open Steam Deck input device");
    return false;
  }

  if (once) {
    Logger_debugLog("Opened Steam Deck input device");
    once = false;
  }

  if (!SDInput_pollState(input)) {
    Logger_infoLog("Polling Steam Deck input device failed, terminating task");
    return false;
  }

  return true;
}
