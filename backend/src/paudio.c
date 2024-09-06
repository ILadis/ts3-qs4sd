
#include "paudio.h"
#include "plugin.h"

static void PAudio_subscriptionCallback(pa_context *context, pa_subscription_event_type_t event, uint32_t index, void *data);
static void PAudio_contextStateCallback(pa_context *context, void *data);

static bool PAudio_setupSubscription(struct PAudio *paudio);

static void PAudio_removeOutputStream(struct PAudio *paudio, int index);
static void PAudio_updateOutputStream(struct PAudio *paudio, int index);

static void PAudio_removeInputStream(struct PAudio *paudio, int index);
static void PAudio_updateInputStream(struct PAudio *paudio, int index);

static void PAudio_removeInputDevice(struct PAudio *paudio, int index);
static void PAudio_updateInputDevice(struct PAudio *paudio, int index);

struct PAudio* PAudio_getInstance() {
  static struct PAudio paudio = {0};
  return &paudio;
}

bool PAudio_connect(struct PAudio *paudio) {
  static const char *name = "TS3 QS4SD PAudio";

  // don't try to reconnect
  if (paudio->context != NULL) {
    return true;
  }

  pa_mainloop *mainloop = NULL;
  pa_mainloop_api *api = NULL;
  pa_context *context = NULL;
  pa_proplist *proplist = NULL;

  mainloop = paudio->mainloop = pa_mainloop_new();
  if (mainloop == NULL) {
    goto error;
  }

  /* Required to get write access to pipewire, see:
   * https://gitlab.freedesktop.org/pipewire/pipewire/-/issues/667
   */
  proplist = paudio->proplist = pa_proplist_new();

  pa_proplist_sets(proplist, "media.type", "Audio");
  pa_proplist_sets(proplist, "media.category", "Manager");
  pa_proplist_sets(proplist, "media.role", "Music");

  api = paudio->api = pa_mainloop_get_api(mainloop);
  context = paudio->context = pa_context_new_with_proplist(api, name, proplist);
  if (context == NULL) {
    goto error;
  }

  pa_context_set_state_callback(context, PAudio_contextStateCallback, paudio);
  if (pa_context_connect(context, NULL, 0, NULL) < 0) {
    goto error;
  }

  return true;

error:
  if (proplist != NULL) {
    pa_proplist_free(proplist);
  }

  if (context != NULL) {
    pa_context_unref(context);
  }

  if (mainloop != NULL) {
    pa_mainloop_free(mainloop);
  }

  paudio->mainloop = NULL;
  paudio->api = NULL;
  paudio->context = NULL;
  paudio->proplist = NULL;

  return false;
}

bool PAudio_runLoop(struct PAudio *paudio) {
  pa_mainloop *mainloop = paudio->mainloop;
  pa_proplist *proplist = paudio->proplist;

  PAudio_guardContextIsset(paudio, false);

  const int timeout = 500;
  int result = pa_mainloop_prepare(mainloop, timeout * 1000);

  if (result < 0) {
    pa_mainloop_free(mainloop);
    pa_proplist_free(proplist);

    paudio->mainloop = NULL;
    paudio->api = NULL;
    paudio->context = NULL;
    paudio->proplist = NULL;

    return false;
  }

  pa_mainloop_poll(mainloop);
  pa_mainloop_dispatch(mainloop);

  int errno = pa_context_errno(paudio->context);
  if (errno != 0) {
    paudio_onError(paudio, pa_strerror(errno));
  }

  return true;
}

void PAudio_shutdown(struct PAudio *paudio) {
  pa_mainloop *mainloop = paudio->mainloop;
  pa_context *context = paudio->context;

  PAudio_guardContextIsset(paudio);

  pa_context_disconnect(context);
  pa_mainloop_run(mainloop, NULL);
}

static void PAudio_quit(struct PAudio *paudio) {
  pa_mainloop_api *api = paudio->api;

  if (api != NULL) {
    api->quit(api, 0);
  }

  paudio->mainloop = NULL;
  paudio->api = NULL;
  paudio->context = NULL;
}

static void PAudio_contextStateCallback(pa_context *context, void *data) {
  struct PAudio *paudio = data;

  switch (pa_context_get_state(context)) {
  case PA_CONTEXT_CONNECTING:
  case PA_CONTEXT_AUTHORIZING:
  case PA_CONTEXT_SETTING_NAME:
    break;

  case PA_CONTEXT_READY:
    PAudio_setupSubscription(paudio);
    paudio_onReady(paudio);
    break;

  case PA_CONTEXT_TERMINATED:
  case PA_CONTEXT_FAILED:
  default:
    PAudio_quit(paudio);
    break;
  }
}

static bool PAudio_setupSubscription(struct PAudio *paudio) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  pa_subscription_mask_t mask = 0
      // applications that are sending audio to sinks (= output devices)
      | PA_SUBSCRIPTION_MASK_SINK_INPUT
      // applications that are receiving audio from sources (= input devices)
      | PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT
      // devices that are audio sources (= input devices)
      | PA_SUBSCRIPTION_MASK_SOURCE;

  pa_context_set_subscribe_callback(context, PAudio_subscriptionCallback, paudio);
  operation = pa_context_subscribe(context, mask, NULL, NULL);
  if (operation != NULL) {
    pa_operation_unref(operation);
    return true;
  }

  return false;
}

static void PAudio_subscriptionCallback(pa_context *context, pa_subscription_event_type_t event, uint32_t index, void *data) {
  struct PAudio *paudio = data;

  int facility = (int) (event & PA_SUBSCRIPTION_EVENT_FACILITY_MASK);
  int type = (int) (event & PA_SUBSCRIPTION_EVENT_TYPE_MASK);

  switch (facility) {
  case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
    if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
      PAudio_removeOutputStream(paudio, index);
    } else {
      PAudio_updateOutputStream(paudio, index);
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
    if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
      PAudio_removeInputStream(paudio, index);
    } else {
      PAudio_updateInputStream(paudio, index);
    }
    break;
  case PA_SUBSCRIPTION_EVENT_SOURCE:
    if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
      PAudio_removeInputDevice(paudio, index);
    } else {
      PAudio_updateInputDevice(paudio, index);
    }
    break;
  }
}

static void PAudioStream_reset(struct PAudioStream *stream) {
  memset(stream, 0, sizeof(*stream));
}

static bool PAudioStream_hasName(struct PAudioStream *stream) {
  return strlen(stream->name) > 0;
}

static bool PAudioStream_changed(struct PAudioStream *stream, struct PAudioStream *other) {
  bool changed = false;

  changed |= stream->index != other->index;
  changed |= stream->muted != other->muted;
  changed |= abs(stream->volume - other->volume) > 0.01;
  changed |= strncmp(stream->name, other->name, sizeof(stream->name));
  changed |= stream->source != other->source;

  return changed;
}

static struct PAudioStream* PAudioStream_next(struct PAudioStream *streams, int length) {
  struct PAudioStream *stream = NULL;

  for (int i = 0; i < length; i++) {
    bool found = streams[i].index == 0;
    if (found) {
      stream = &streams[i];
      break;
    }
  }

  return stream;
}

static struct PAudioStream* PAudioStream_byIndex(struct PAudioStream *streams, int length, int index) {
  struct PAudioStream *stream = NULL;

  for (int i = 0; i < length; i++) {
    bool found = streams[i].index == index;
    if (found) {
      stream = &streams[i];
      break;
    }
  }

  return stream;
}

static void PAudioStream_updateFromSinkInput(struct PAudioStream *output, const pa_sink_input_info *info) {
  output->index = info->index;
  output->muted = info->mute == 1;

  const pa_cvolume *volume = &info->volume;
  const pa_proplist *proplist = info->proplist;

  if (volume->channels > 0) {
    double percentage = PAudio_volumeToPercentage(volume->values[0]);
    output->volume = percentage;
  }

  const char *name = pa_proplist_gets(proplist, PA_PROP_APPLICATION_NAME);
  if (name != NULL) {
    snprintf(output->name, sizeof(output->name), "%s", name);
  }
}

static void PAudioStream_updateFromSourceOutput(struct PAudioStream *input, const pa_source_output_info *info) {
  input->index = info->index;
  input->muted = info->mute == 1;

  const pa_cvolume *volume = &info->volume;
  const pa_proplist *proplist = info->proplist;

  if (volume->channels > 0) {
    double percentage = PAudio_volumeToPercentage(volume->values[0]);
    input->volume = percentage;
  }

  const char *name = pa_proplist_gets(proplist, PA_PROP_APPLICATION_NAME);
  if (name != NULL) {
    snprintf(input->name, sizeof(input->name), "%s", name);
  }
}

static void PAudioDevice_reset(struct PAudioDevice *device) {
  memset(device, 0, sizeof(*device));
}

static bool PAudioDevice_hasName(struct PAudioDevice *device) {
  return strlen(device->name) > 0;
}

static bool PAudioDevice_isUsable(struct PAudioDevice *device) {
  return device->usable == true;
}

static bool PAudioDevice_changed(struct PAudioDevice *device, struct PAudioDevice *other) {
  bool changed = false;

  changed |= device->index != other->index;
  changed |= strncmp(device->name, other->name, sizeof(device->name));

  return changed;
}

static struct PAudioDevice* PAudioDevice_next(struct PAudioDevice *devices, int length) {
  struct PAudioDevice *device = NULL;

  for (int i = 0; i < length; i++) {
    bool found = devices[i].index == 0;
    if (found) {
      device = &devices[i];
      break;
    }
  }

  return device;
}

static struct PAudioDevice* PAudioDevice_byId(struct PAudioDevice *devices, int length, int id) {
  struct PAudioDevice *device = NULL;

  for (int i = 0; i < length; i++) {
    bool found = devices[i].id == id;
    if (found) {
      device = &devices[i];
      break;
    }
  }

  return device;
}

static struct PAudioDevice* PAudioDevice_byIndex(struct PAudioDevice *devices, int length, int index) {
  struct PAudioDevice *device = NULL;

  for (int i = 0; i < length; i++) {
    bool found = devices[i].index == index;
    if (found) {
      device = &devices[i];
      break;
    }
  }

  return device;
}

static void PAudioDevice_updateFromSink(struct PAudioDevice *device, const pa_source_info *info) {
  device->id = info->card;
  device->index = info->index;
  device->usable = info->active_port != NULL;

  const pa_proplist *proplist = info->proplist;
  const char *name = pa_proplist_gets(proplist, PA_PROP_DEVICE_DESCRIPTION);

  if (name != NULL) {
    snprintf(device->name, sizeof(device->name), "%s", name);
  }
}

static struct PAudioStream* PAudio_getNextAudioOutputStream(struct PAudio *paudio) {
  return PAudioStream_next(paudio->streams.output, length(paudio->streams.output));
}

static struct PAudioStream* PAudio_getAudioOutputStreamByIndex(struct PAudio *paudio, int index) {
  return PAudioStream_byIndex(paudio->streams.output, length(paudio->streams.output), index);
}

static void PAudio_sinkInputInfoCallback(pa_context *context, const pa_sink_input_info *info, int last, void *data) {
  struct PAudio *paudio = data;

  PAudio_guardLastCallback(last);

  struct PAudioStream target = {0};
  PAudioStream_updateFromSinkInput(&target, info);

  if (!PAudioStream_hasName(&target)) {
    return; // only add output streams which have an application name associated with it
  }

  struct PAudioStream *output = PAudio_getAudioOutputStreamByIndex(paudio, info->index);
  if (output == NULL) {
    output = PAudio_getNextAudioOutputStream(paudio);
  }

  if (output != NULL) {
    if (PAudioStream_changed(output, &target)) {
      *output = target;
      paudio_onOutputStreamsChanged(paudio);
    }
  }
}

static void PAudio_removeOutputStream(struct PAudio *paudio, int index) {
  struct PAudioStream *output = PAudio_getAudioOutputStreamByIndex(paudio, index);
  if (output != NULL) {
    PAudioStream_reset(output);
    paudio_onOutputStreamsChanged(paudio);
  }
}

static void PAudio_updateOutputStream(struct PAudio *paudio, int index) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio);

  operation = pa_context_get_sink_input_info(context, index, PAudio_sinkInputInfoCallback, paudio);
  if (operation != NULL) {
    pa_operation_unref(operation);
  }
}

void PAudio_updateOutputStreams(struct PAudio *paudio) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio);

  operation = pa_context_get_sink_input_info_list(context, PAudio_sinkInputInfoCallback, paudio);
  if (operation != NULL) {
    pa_operation_unref(operation);
  }
}

static void PAudio_setSinkInputVolumeCallback(pa_context *context, const pa_sink_input_info *info, int last, void *data) {
  struct PAudio *paudio = data;
  pa_operation *operation = NULL;

  PAudio_guardLastCallback(last);

  struct PAudioStream *output = PAudio_getAudioOutputStreamByIndex(paudio, info->index);
  if (output == NULL) {
    return paudio_onError(paudio, "failed to get PAudioStream (output) by index");
  }

  int channels = info->channel_map.channels;
  int value = PAudio_percentageToVolume(output->volume);

  pa_cvolume volume = { .channels = 1 };
  pa_cvolume_set(&volume, channels, value);

  operation = pa_context_set_sink_input_volume(context, info->index, &volume, NULL, NULL);
  if (operation != NULL) {
    pa_operation_unref(operation);
  }
}

bool PAudio_setOutputStreamVolume(struct PAudio *paudio, int index, double volume) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio, false);

  struct PAudioStream *output = PAudio_getAudioOutputStreamByIndex(paudio, index);
  if (output == NULL) {
    return false;
  }

  output->volume = volume;

  operation = pa_context_get_sink_input_info(context, index, PAudio_setSinkInputVolumeCallback, paudio);
  if (operation != NULL) {
    pa_operation_unref(operation);
    return true;
  }

  return false;
}

bool PAudio_nextOutputStream(struct PAudio *paudio, struct PAudioStream **output) {
  struct PAudioStream *previous = *output, *next = NULL;
  int last = length(paudio->streams.output) - 1;

  if (previous == NULL) {
    next = &paudio->streams.output[0];
  } else {
    next = previous + 1;
  }

  do {
    if (next->index != 0) {
      *output = next;
      return true;
    }

    next++;
  } while (next != &paudio->streams.output[last]);

  return false;
}

static struct PAudioStream* PAudio_getNextAudioInputStream(struct PAudio *paudio) {
  return PAudioStream_next(paudio->streams.input, length(paudio->streams.input));
}

static struct PAudioStream* PAudio_getAudioInputStreamByIndex(struct PAudio *paudio, int index) {
  return PAudioStream_byIndex(paudio->streams.input, length(paudio->streams.input), index);
}

static struct PAudioDevice* PAudio_getAudioInputDeviceByIndex(struct PAudio *paudio, int index);

static void PAudio_sourceOutputInfoCallback(pa_context *context, const pa_source_output_info *info, int last, void *data) {
  struct PAudio *paudio = data;

  PAudio_guardLastCallback(last);

  struct PAudioStream target = {0};
  PAudioStream_updateFromSourceOutput(&target, info);

  if (!PAudioStream_hasName(&target)) {
    return; // only add input streams which have an application name associated with it
  }

  struct PAudioDevice *source = PAudio_getAudioInputDeviceByIndex(paudio, info->source);
  target.source = source;

  struct PAudioStream *input = PAudio_getAudioInputStreamByIndex(paudio, info->index);
  if (input == NULL) {
    input = PAudio_getNextAudioInputStream(paudio);
  }

  if (input != NULL) {
    if (PAudioStream_changed(input, &target)) {
      *input = target;
      paudio_onInputStreamsChanged(paudio);
    }
  }
}

static void PAudio_removeInputStream(struct PAudio *paudio, int index) {
  struct PAudioStream *input = PAudio_getAudioInputStreamByIndex(paudio, index);
  if (input != NULL) {
    PAudioStream_reset(input);
    paudio_onInputStreamsChanged(paudio);
  }
}

static void PAudio_updateInputStream(struct PAudio *paudio, int index) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio);

  operation = pa_context_get_source_output_info(context, index, PAudio_sourceOutputInfoCallback, paudio);
  if (operation != NULL) {
    pa_operation_unref(operation);
  }
}

void PAudio_updateInputStreams(struct PAudio *paudio) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio);

  operation = pa_context_get_source_output_info_list(context, PAudio_sourceOutputInfoCallback, paudio);
  if (operation != NULL) {
    pa_operation_unref(operation);
  }
}

bool PAudio_findInputStream(struct PAudio *paudio, struct PAudioStream **input, const char *name) {
  for (int i = 0; i < length(paudio->streams.input); i++) {
    struct PAudioStream *stream = &paudio->streams.input[i];
    bool found = strncmp(stream->name, name, sizeof(stream->name)) == 0;

    if (found) {
      *input = stream;
      return true;
    }
  }

  return false;
}

bool PAudio_changeInputStreamSourceDevice(struct PAudio *paudio, struct PAudioStream *input, int index) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio, false);

  struct PAudioDevice *device = PAudio_getAudioInputDeviceByIndex(paudio, index);
  if (device == NULL) {
    return false;
  }

  operation = pa_context_move_source_output_by_index(context, input->index, device->index, NULL, NULL);
  if (operation != NULL) {
    pa_operation_unref(operation);
    return true;
  }

  return false;
}

static struct PAudioDevice* PAudio_getNextAudioInputDevice(struct PAudio *paudio) {
  return PAudioDevice_next(paudio->devices.input, length(paudio->devices.input));
}

static struct PAudioDevice* PAudio_getAudioInputDeviceById(struct PAudio *paudio, int id) {
  return PAudioDevice_byId(paudio->devices.input, length(paudio->devices.input), id);
}

static struct PAudioDevice* PAudio_getAudioInputDeviceByIndex(struct PAudio *paudio, int index) {
  return PAudioDevice_byIndex(paudio->devices.input, length(paudio->devices.input), index);
}

static void PAudio_sourceInfoCallback(pa_context *context, const pa_source_info *info, int last, void *data) {
  struct PAudio *paudio = data;

  PAudio_guardLastCallback(last);

  struct PAudioDevice target = {0};
  PAudioDevice_updateFromSink(&target, info);

  if (!PAudioDevice_isUsable(&target) || !PAudioDevice_hasName(&target)) {
    return; // only add input devices that are "usable" and have names
  }

  struct PAudioDevice *input = PAudio_getAudioInputDeviceById(paudio, info->card);
  if (input == NULL) {
    input = PAudio_getNextAudioInputDevice(paudio);
  }

  if (input != NULL) {
    if (PAudioDevice_changed(input, &target)) {
      *input = target;
      paudio_onInputDevicesChanged(paudio);
    }
  }
}

static void PAudio_removeInputDevice(struct PAudio *paudio, int index) {
  struct PAudioDevice *input = PAudio_getAudioInputDeviceByIndex(paudio, index);
  if (input != NULL) {
    PAudioDevice_reset(input);
    paudio_onInputDevicesChanged(paudio);
  }
}

static void PAudio_updateInputDevice(struct PAudio *paudio, int index) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio);

  operation = pa_context_get_source_info_by_index(context, index, PAudio_sourceInfoCallback, paudio);
  if (operation != NULL) {
    pa_operation_unref(operation);
  }
}

void PAudio_updateInputDevices(struct PAudio *paudio) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio);

  operation = pa_context_get_source_info_list(context, PAudio_sourceInfoCallback, paudio);
  if (operation != NULL) {
    pa_operation_unref(operation);
  }
}

bool PAudio_nextInputDevice(struct PAudio *paudio, struct PAudioDevice **input) {
  struct PAudioDevice *previous = *input, *next = NULL;
  int last = length(paudio->devices.input) - 1;

  if (previous == NULL) {
    next = &paudio->devices.input[0];
  } else {
    next = previous + 1;
  }

  do {
    if (next->index != 0) {
      *input = next;
      return true;
    }

    next++;
  } while (next != &paudio->devices.input[last]);

  return false;
}
