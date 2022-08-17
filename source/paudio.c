
#include <stdio.h>
#include "paudio.h"

#define length(array) (sizeof(array)/sizeof(array[0]))

static void PAudio_subscriptionCallback(pa_context *context, pa_subscription_event_type_t event, uint32_t index, void *data);
static void PAudio_contextStateCallback(pa_context *context, void *data);

static bool PAudio_setupSubscription(struct PAudio *paudio);

struct PAudio* PAudio_getInstance() {
  static struct PAudio paudio = {0};

  if (paudio.context != NULL) {
    return &paudio;
  }

  pa_mainloop *mainloop = NULL;
  pa_mainloop_api *api = NULL;
  pa_context *context = NULL;

  mainloop = paudio.mainloop = pa_mainloop_new();
  if (mainloop == NULL) {
    goto error;
  }

  api = paudio.api = pa_mainloop_get_api(mainloop);
  context = paudio.context = pa_context_new(api, NULL);
  if (context == NULL) {
    goto error;
  }

  pa_context_set_state_callback(context, PAudio_contextStateCallback, &paudio);
  if (pa_context_connect(context, NULL, 0, NULL) < 0) {
    goto error;
  }

  return &paudio;

error:
  if (context != NULL) {
    pa_context_unref(context);
  }

  if (mainloop != NULL) {
    pa_mainloop_free(mainloop);
  }

  paudio.mainloop = NULL;
  paudio.api = NULL;
  paudio.context = NULL;

  return &paudio;
}

bool PAudio_runLoop(struct PAudio *paudio) {
  pa_mainloop *mainloop = paudio->mainloop;

  PAudio_guardContextIsset(paudio, false);
  int result = pa_mainloop_iterate(mainloop, 0, NULL);

  if (result < 0) {
    pa_mainloop_free(mainloop);

    paudio->mainloop = NULL;
    paudio->api = NULL;
    paudio->context = NULL;

    return false;
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

  pa_context_set_subscribe_callback(context, PAudio_subscriptionCallback, paudio);
  operation = pa_context_subscribe(context, (pa_subscription_mask_t) (PA_SUBSCRIPTION_MASK_SINK_INPUT), NULL, NULL);
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
    // TODO should we cancel any ongoing operations?
    // TODO there may be a burst of sink input events, reduce "paudio_onOutputsChanged" callbacks
    if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
      PAudio_updateOutputs(paudio);
    }
    else {
      PAudio_updateOutput(paudio, index);
    }
    break;
  }
}

static void PAudioOutput_reset(struct PAudioOutput *output) {
  memset(output, 0, sizeof(*output));
}

static void PAudioOutput_update(struct PAudioOutput *output, const pa_sink_input_info *info) {
  output->index = info->index;
  output->muted = info->mute == 1;

  const pa_cvolume *volume = &info->volume;
  const pa_proplist *proplist = info->proplist;

  if (volume->channels > 0) {
    double percentage = PAudio_volumeToPercentage(volume->values[0]);
    output->volume = percentage;
  }

  const char *name = pa_proplist_gets(proplist, "application.name");
  if (name != NULL) {
    strcpy(output->name, name);
  }
}

static struct PAudioOutput* PAudio_getNextAudioOutput(struct PAudio *paudio) {
  struct PAudioOutput *output = NULL;

  for (int i = 0; i < length(paudio->outputs); i++) {
    bool found = paudio->outputs[i].index == 0;
    if (found) {
      output = &paudio->outputs[i];
      break;
    }
  }

  return output;
}

static struct PAudioOutput* PAudio_getAudioOutputByIndex(struct PAudio *paudio, int index) {
  struct PAudioOutput *output = NULL;

  for (int i = 0; i < length(paudio->outputs); i++) {
    bool found = paudio->outputs[i].index == index;
    if (found) {
      output = &paudio->outputs[i];
      break;
    }
  }

  return output;
}

static void PAudio_sinkInputInfoCallback(pa_context *context, const pa_sink_input_info *info, int last, void *data) {
  struct PAudio *paudio = data;

  if (last) {
    return paudio_onOutputsChanged(paudio);
  }

  struct PAudioOutput *output = PAudio_getAudioOutputByIndex(paudio, info->index);
  if (output == NULL) {
    output = PAudio_getNextAudioOutput(paudio);
  }

  PAudioOutput_update(output, info);
}

bool PAudio_updateOutput(struct PAudio *paudio, int index) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio, false);

  operation = pa_context_get_sink_input_info(context, index, PAudio_sinkInputInfoCallback, paudio);
  if (operation) {
    pa_operation_unref(operation);
    return true;
  }

  return false;
}

bool PAudio_updateOutputs(struct PAudio *paudio) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio, false);

  for (int i = 0; i < length(paudio->outputs); i++) {
    PAudioOutput_reset(&paudio->outputs[i]);
  }

  operation = pa_context_get_sink_input_info_list(context, PAudio_sinkInputInfoCallback, paudio);
  if (operation) {
    pa_operation_unref(operation);
    return true;
  }

  return false;
}

static void PAudio_setSinkInputVolumeCallback(pa_context *context, const pa_sink_input_info *info, int last, void *data) {
  struct PAudio *paudio = data;

  if (last) {
    return paudio_onOutputsChanged(paudio);
  }

  struct PAudioOutput *output = PAudio_getAudioOutputByIndex(paudio, info->index);
  if (output == NULL) {
    return paudio_onError(paudio);
  }

  pa_cvolume volume = info->volume;

  int channels = info->channel_map.channels;
  int value = PAudio_percentageToVolume(output->volume);

  pa_cvolume_set(&volume, channels, value);
  pa_operation_unref(pa_context_set_sink_input_volume(context, info->index, &volume, NULL, NULL));
}

bool PAudio_setOutputVolume(struct PAudio *paudio, int index, double volume) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  PAudio_guardContextIsset(paudio, false);

  struct PAudioOutput *output = PAudio_getAudioOutputByIndex(paudio, index);
  if (output == NULL) {
    return false;
  }

  output->volume = volume;

  operation = pa_context_get_sink_input_info(context, index, PAudio_setSinkInputVolumeCallback, paudio);
  if (operation) {
    pa_operation_unref(operation);
    return true;
  }

  return false;
}

