
#include "states.h"

static void Injector_fetchRetryFn(void *context) {
  struct Injector *injector = (struct Injector *) context;
  Injector_gotoFetchWSUrlState(injector);
}

void Injector_gotoRetryState(struct Injector *injector) {
  injector->state = STATE_RETRY;
  injector->timer = mg_timer_add(injector->manager, 3000, 0, Injector_fetchRetryFn, injector);
}