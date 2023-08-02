
#include "executor.h"

static void* Executor_runner(void *context) {
  struct Executor *executor = context;
  Task task = executor->task;

  while (executor->running) {
    if (task() == false) {
      executor->running = false;
      break;
    }
  }

  return NULL;
}

void Executor_start(struct Executor *executor) {
  pthread_t *thread = &executor->thread;

  if (!executor->running) {
    int result = pthread_create(thread, NULL, Executor_runner, (void*) executor);
    executor->running = result == 0;
  }
}

void Executor_stop(struct Executor *executor) {
  executor->running = false;

  pthread_t thread = executor->thread;
  pthread_join(thread, NULL);
}
