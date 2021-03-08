#pragma once
#include <ucos_ii.h>

struct Task {
  typedef void (*TaskFn)(void*);
  INT8U priority;
  TaskFn task;
  void* arg;
  OS_STK *stack;
};

#define SIZE_ARR(arr) (sizeof(arr)/sizeof(arr[0]))
#define CREATE_TASK(prio, task, arg, stack) Task{ prio, task, arg, &stack[SIZE_ARR(stack)-1] }

