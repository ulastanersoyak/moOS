#ifndef MUTEX_H
#define MUTEX_H

#include "task.h"
#include <stdint.h>

// mutexes are global and found by name, so separate programs can share one.
// a task gets an id from mutex_open and uses it for lock and unlock. the
// mutex goes away when the last task that opened it closes it or exits
#define MAX_MUTEXES 32
#define MUTEX_NAME_LEN 16

// mutex_open flags, only used by the open that creates the mutex
#define MUTEX_INHERIT 0x1 // priority inheritance

// returns the id of the mutex called name, creating it if needed, or a
// negative error
int32_t mutex_open (const char *name, uint32_t flags);

// waits until the running task owns the mutex. with priority inheritance
// the owner runs at the priority of the most important waiter until it
// unlocks. returns TASK_KILLED if the task got killed while waiting
int32_t mutex_lock (int32_t id);

// hands the mutex to the highest priority waiter
int32_t mutex_unlock (int32_t id);

int32_t mutex_close (int32_t id);

// unlocks and closes every mutex of a task that exits
void mutex_release_all (struct task *task);

// recomputes the priority of task from its base priority and the waiters
// of the mutexes it owns, e.g. after the base priority changed
void mutex_update_priority (struct task *task);

#endif // !MUTEX_H
