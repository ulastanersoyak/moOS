#ifndef SCHED_H
#define SCHED_H

#include "task.h"
#include <stdint.h>

enum sched_policy
{
  // every ready task runs for SCHED_QUANTUM_TICKS in turn, priorities are
  // ignored
  SCHED_ROUND_ROBIN,
  // fixed priority preemptive: the highest priority ready task always runs
  // and preempts lower priority tasks as soon as it becomes ready. tasks
  // with equal priority share the cpu round robin
  SCHED_REAL_TIME,
};

void sched_set_policy (enum sched_policy policy);
enum sched_policy sched_get_policy (void);
const char *sched_policy_name (enum sched_policy policy);

// switches to the task picked by the policy. interrupts must be disabled and
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
