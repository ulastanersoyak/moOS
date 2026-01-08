#include "sched.h"
#include "../../drivers/timer/pit.h"
#include "../config.h"
#include "tss.h"

// asm functions
void context_switch (uint32_t *old_esp, uint32_t new_esp);

static enum sched_policy current_policy = SCHED_ROUND_ROBIN;
static volatile uint8_t need_resched = 0;

void
sched_set_policy (enum sched_policy policy)
{
  current_policy = policy;
  need_resched = 1;
}

enum sched_policy
sched_get_policy (void)
{
  return current_policy;
}

const char *
sched_policy_name (enum sched_policy policy)
{
  switch (policy)
    {
    case SCHED_REAL_TIME:
      return "real time (fixed priority preemptive)";
    case SCHED_EDF:
      return "earliest deadline first";
    default:
      return "round robin";
    }
}

// edf ranks: the shell (and killed tasks) first, then periodic tasks, then
// the rest
static uint8_t
sched_edf_class (struct task *task)
{
  if (task->priority >= TASK_PRIORITY_SHELL)
    {
      return 0;
    }
  return task->period_ticks ? 1 : 2;
}

// true if task a should run before task b under the current policy
static uint8_t
sched_runs_before (struct task *a, struct task *b)
{
  if (current_policy == SCHED_EDF)
    {
      uint8_t class_a = sched_edf_class (a);
      uint8_t class_b = sched_edf_class (b);
      if (class_a != class_b)
        {
          return class_a < class_b;
        }
      if (class_a == 1)
        {
          return (int32_t)(a->deadline - b->deadline) < 0;
        }
    }
  return a->priority > b->priority;
}

static uint8_t
task_runnable (struct task *task)
{
  return task->state == TASK_READY
         || (task == task_running && task->state == TASK_RUNNING);
}

// scans the task table in circular order starting after the running task,
// so the running task is the last candidate and equal tasks take turns
static struct task *
sched_pick_next (void)
{
  uint32_t current = task_running - task_table;
  struct task *best = 0;
  for (uint32_t n = 1; n <= MAX_TASKS; n++)
    {
      struct task *task = &task_table[(current + n) % MAX_TASKS];
      if (task == task_idle || !task_runnable (task))
        {
          continue;
        }
      if (current_policy == SCHED_ROUND_ROBIN)
        {
          return task;
        }
      if (!best || sched_runs_before (task, best))
        {
          best = task;
        }
    }
  return best ? best : task_idle;
}

void
schedule (void)
{
  need_resched = 0;
  struct task *prev = task_running;
  struct task *next = sched_pick_next ();
  next->slice = SCHED_QUANTUM_TICKS;
  if (next == prev)
    {
      prev->state = TASK_RUNNING;
      return;
    }

  if (prev->state == TASK_RUNNING)
    {
      prev->state = TASK_READY;
    }
  next->state = TASK_RUNNING;
  task_running = next;

  // interrupts from ring 3 land on the kernel stack of the task
  if (next->kernel_stack)
    {
      tss.esp0 = next->kernel_stack_top;
    }
  switch_page_dir (next->dir ? next->dir->dir_entry
                             : task_kernel_dir ()->dir_entry);
  context_switch (&prev->context_esp, next->context_esp);

  // prev runs again from here. the stack of a task that exited is no longer
  // in use, so it can be freed now
  task_reap_zombies ();
}

void
sched_wake (struct task *task)
{
  task->state = TASK_READY;
  if (task_running == task_idle
      || (current_policy != SCHED_ROUND_ROBIN
          && sched_runs_before (task, task_running)))
    {
      need_resched = 1;
    }
}

void
sched_tick (void)
{
  uint32_t now = pit_ticks ();
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->state == TASK_SLEEPING
          && (int32_t)(now - task->wake_tick) >= 0)
        {
          sched_wake (task);
        }
    }

  task_running->cpu_ticks++;
  if (task_running != task_idle && --task_running->slice <= 0)
    {
      need_resched = 1;
    }
}

void
sched_request_resched (void)
{
  need_resched = 1;
}

uint8_t
sched_need_resched (void)
{
  return need_resched;
}
