#ifndef SCHED_H
#define SCHED_H

#include "task.h"
#include <stdint.h>

// switches to the next ready task in turn. interrupts must be disabled and
// the running task must already be in its new state (e.g. TASK_SLEEPING)
void schedule (void);

// called from irq0. wakes sleeping tasks and accounts the time slice
void sched_tick (void);

// makes a blocked task ready and asks for a switch if it should preempt the
// running task
void sched_wake (struct task *task);

// asks for a switch at the next return to user mode
void sched_request_resched (void);

uint8_t sched_need_resched (void);

#endif // !SCHED_H
