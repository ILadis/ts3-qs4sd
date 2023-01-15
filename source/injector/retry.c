
#include "states.h"

static void Injector_fetchRetryFn(void *context) {
  struct Injector *injector = (struct Injector *) context;
  Injector_gotoFetchWSUrlState(injector);
}

void Injector_gotoRetryState(struct Injector *injector) {
  const int timer = 3000;
  Logger_debugLog("Retrying injection after %ld ms...", timer);

  injector->state = STATE_RETRY;
  injector->timer = mg_timer_add(injector->manager, timer, 0, Injector_fetchRetryFn, injector);
}