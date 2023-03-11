
#include "executor.h"

struct Executor* Executor_getInstance() {
  static struct Executor executor = {0};
  return &executor;
}

void Executor_addTask(
    struct Executor *executor,
    Task task)
{
  Task *tasks = executor->tasks;

  if (!executor->running) {
    int index = 0;
    const int limit = sizeof(executor->tasks) / sizeof(*tasks);

    while (tasks[index] != NULL && index < limit) {
      index++;
    }

    tasks[index] = task;
  }
}

static void* Executor_runner(void *context) {
  struct Executor *executor = context;
  Task *tasks = executor->tasks;

  while (executor->running) {
    int index = 0;
    const int limit = sizeof(executor->tasks) / sizeof(*tasks);

    while (tasks[index] != NULL && index < limit) {
      tasks[index++]();
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
