#ifndef PAUDIO_H
#define PAUDIO_H

#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <pulse/pulseaudio.h>

/* Usage see "sample" code here:
 * https://gitlab.freedesktop.org/pulseaudio/pulseaudio/-/blob/master/src/utils/pactl.c
 */

struct PAudio {
  pa_mainloop_api *api;
  pa_context *context;

  struct PAudioOutput {
    int index;
    char name[255];
    double volume;
    bool muted;
  } outputs[10];
};

struct PAudio* PAudio_getInstance();

bool PAudio_updateOutputs(struct PAudio *paudio);
bool PAudio_setOutputVolume(struct PAudio *paudio, int index, double volume);

#endif
