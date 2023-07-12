#ifndef STATES_H
#define STATES_H

#include "../settings.h"

#include "../server.h"
#include "../injector.h"

#include <stdio.h>
#include <string.h>

void Injector_gotoFetchWSUrlState(struct Injector *injector);
void Injector_gotoEvaluateJSCodeState(struct Injector *injector);
void Injector_gotoRetryState(struct Injector *injector);

#endif
