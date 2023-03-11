#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>
#include <pthread.h>

typedef void (*Task)();

struct Executor {
  Task tasks[10];
  pthread_t thread;
  volatile bool running;
};

struct Executor* Executor_getInstance();

void Executor_addTask(
    struct Executor *executor,
    Task task);

void Executor_start(struct Executor *executor);
void Executor_stop(struct Executor *executor);

#endif
