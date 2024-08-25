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

struct PAudioStream {
  int index;
  char name[250];
  double volume;
  bool muted;
  struct PAudioDevice *source;
};

struct PAudioDevice {
  int id;
  int index;
  char name[250];
};

struct PAudio {
  pa_mainloop *mainloop;
  pa_mainloop_api *api;
  pa_context *context;
  pa_proplist *proplist;

  struct {
    struct PAudioStream output[10]; // playback
    struct PAudioStream input[10]; // recording
  } streams;

  struct {
    struct PAudioDevice input[10];
  } devices;
};

#define PAudio_guardContextIsset(paudio, ...) do { if (paudio->context == NULL) return __VA_ARGS__; } while(0);
#define PAudio_guardLastCallback(last, ...) do { if (last) return __VA_ARGS__; } while(0);

#define PAudio_volumeToPercentage(value) (((double) value * 100) / PA_VOLUME_NORM)
#define PAudio_percentageToVolume(value) (((double) value * PA_VOLUME_NORM) / 100)

struct PAudio* PAudio_getInstance();
bool PAudio_connect(struct PAudio *paudio);
bool PAudio_runLoop(struct PAudio *paudio);
void PAudio_shutdown(struct PAudio *paudio);

void PAudio_updateOutputStreams(struct PAudio *paudio);
void PAudio_updateInputStreams(struct PAudio *paudio);
void PAudio_updateInputDevices(struct PAudio *paudio);

bool PAudio_nextOutputStream(struct PAudio *paudio, struct PAudioStream **output);
bool PAudio_setOutputStreamVolume(struct PAudio *paudio, int index, double volume);

bool PAudio_findInputStream(struct PAudio *paudio, struct PAudioStream **input, const char *name);
bool PAudio_changeInputStreamSourceDevice(struct PAudio *paudio, struct PAudioStream *input, int index);

bool PAudio_nextInputDevice(struct PAudio *paudio, struct PAudioDevice **input);

// external callbacks
extern void paudio_onReady(struct PAudio *paudio);
extern void paudio_onOutputStreamsChanged(struct PAudio *paudio);
extern void paudio_onInputStreamsChanged(struct PAudio *paudio);
extern void paudio_onInputDevicesChanged(struct PAudio *paudio);
extern void paudio_onError(struct PAudio *paudio, const char *message);

#endif
