
#include <stdio.h>
#include "paudio.h"

#define length(array) (sizeof(array)/sizeof(array[0]))

struct PAudio* PAudio_getInstance() {
  static struct PAudio instance = {0};
  return &instance;
}

static void PAudio_disconnect(struct PAudio *paudio) {
  pa_context *context = paudio->context;
  paudio->context = NULL;

  if (context != NULL) {
    pa_context_disconnect(context);
  }
}

static void PAudio_quit(struct PAudio *paudio) {
  pa_mainloop_api *api = paudio->api;
  paudio->api = NULL;

  if (api != NULL) {
    api->quit(api, 0);
  }
}

static void PAudioOutput_reset(struct PAudioOutput *output) {
  output->index = 0;
  output->name[0] = '\0';
  output->volume = 0.0;
  output->muted = false;
}

static void PAudioOutput_update(struct PAudioOutput *output, const pa_sink_input_info *info) {
  output->index = info->index;
  output->muted = info->mute == 1;

  const pa_cvolume *volume = &info->volume;
  const pa_proplist *proplist = info->proplist;

  if (volume->channels > 0) {
    double value = (double) volume->values[0];
    double percentage = (value * 100) / PA_VOLUME_NORM;
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
    bool next = paudio->outputs[i].index == 0;
    if (next) {
      output = &paudio->outputs[i];
      break;
    }
  }

  return output;
}

static void PAudio_sinkInputInfoCallback(pa_context *context, const pa_sink_input_info *info, int last, void *data) {
  struct PAudio *paudio = data;

  if (last) {
    return PAudio_disconnect(paudio);
  }

  struct PAudioOutput *output = PAudio_getNextAudioOutput(paudio);
  PAudioOutput_update(output, info);
}

static void PAudio_requestSinkInputInfo(struct PAudio *paudio) {
  pa_context *context = paudio->context;
  pa_operation *operation = NULL;

  for (int i = 0; i < length(paudio->outputs); i++) {
    PAudioOutput_reset(&paudio->outputs[i]);
  }

  operation = pa_context_get_sink_input_info_list(context, PAudio_sinkInputInfoCallback, paudio);
  if (operation) {
    pa_operation_unref(operation);
  }
}

static void PAudio_contextStateCallback(pa_context *context, void *data) {
  struct PAudio *paudio = data;

  switch (pa_context_get_state(context)) {
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;

    case PA_CONTEXT_READY:
      PAudio_requestSinkInputInfo(paudio);
      break;

    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_FAILED:
    default:
      PAudio_quit(paudio);
      break;
  }
}

bool PAudio_updateOutputs(struct PAudio *paudio) {
  pa_mainloop *mainloop = pa_mainloop_new();

  pa_mainloop_api *api = NULL;
  pa_context *context = NULL;

  if (mainloop == NULL) {
    goto error;
  }

  api = paudio->api = pa_mainloop_get_api(mainloop);
  context = paudio->context = pa_context_new(api, NULL);

  if (context == NULL) {
    goto error;
  }

  pa_context_set_state_callback(context, PAudio_contextStateCallback, paudio);
  if (pa_context_connect(context, NULL, 0, NULL) < 0) {
    goto error;
  }

  pa_mainloop_run(mainloop, NULL);
  return true;

error:
  if (context != NULL) {
    pa_context_unref(context);
    paudio->context =  NULL;
  }

  if (mainloop != NULL) {
    pa_mainloop_free(mainloop);
  }

  return false;
}

bool PAudio_setOutputVolume(struct PAudio *paudio, int index, double volume) {
  // pa_context_get_sink_input_info(c, sink_input_idx, get_sink_input_volume_callback, NULL);
  return false;
}

