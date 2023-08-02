#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

typedef bool (*Task)();

struct Executor {
  Task task;
  pthread_t thread;
  volatile bool running;
};

#define Executor_forTask(task) ((struct Executor) { task })

void Executor_start(struct Executor *executor);
void Executor_stop(struct Executor *executor);

#endif
