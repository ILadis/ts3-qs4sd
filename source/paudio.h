#ifndef PAUDIO_H
#define PAUDIO_H

#include <stddef.h>
#include <stdbool.h>

#include <stdio.h>
#include <string.h>

#include <pulse/pulseaudio.h>

#define PAUDIO_USER_EVENT 200

/* Usage see "sample" code here:
 * https://gitlab.freedesktop.org/pulseaudio/pulseaudio/-/blob/master/src/utils/pactl.c
 */

struct PAudio {
  pa_mainloop *mainloop;
  pa_mainloop_api *api;
  pa_context *context;
  pa_proplist *proplist;

  struct PAudioOutput {
    int index;
    char name[250];
    double volume;
    bool muted;
  } outputs[10];
};

#define PAudio_guardContextIsset(paudio, ...) do { if (paudio->context == NULL) return __VA_ARGS__; } while(0);
#define PAudio_guardLastCallback(last, ...) do { if (last) return __VA_ARGS__; } while(0);

#define PAudio_volumeToPercentage(value) (((double) value * 100) / PA_VOLUME_NORM)
#define PAudio_percentageToVolume(value) (((double) value * PA_VOLUME_NORM) / 100)

struct PAudio* PAudio_getInstance();
bool PAudio_connect(struct PAudio *paudio);
bool PAudio_runLoop(struct PAudio *paudio);
void PAudio_shutdown(struct PAudio *paudio);

bool PAudio_updateOutput(struct PAudio *paudio, int index);
bool PAudio_updateOutputs(struct PAudio *paudio);
bool PAudio_nextOutput(struct PAudio *paudio, struct PAudioOutput **output);

bool PAudio_setOutputVolume(struct PAudio *paudio, int index, double volume);

// external callbacks
extern void paudio_onReady(struct PAudio *paudio);
extern void paudio_onOutputsChanged(struct PAudio *paudio);
extern void paudio_onError(struct PAudio *paudio, const char *message);

#endif
