#include "sched.h"
#include "../../drivers/timer/pit.h"
#include "../config.h"
#include "tss.h"

// asm functions
void context_switch (uint32_t *old_esp, uint32_t new_esp);

static volatile uint8_t need_resched = 0;

static uint8_t
task_runnable (struct task *task)
{
  return task->state == TASK_READY
         || (task == task_running && task->state == TASK_RUNNING);
}

// scans the task table in circular order starting after the running task,
// so the running task is the last candidate and every task takes its turn
static struct task *
sched_pick_next (void)
{
  uint32_t current = task_running - task_table;
  for (uint32_t n = 1; n <= MAX_TASKS; n++)
    {
      struct task *task = &task_table[(current + n) % MAX_TASKS];
      if (task != task_idle && task_runnable (task))
        {
          return task;
        }
    }
  return task_idle;
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
  if (task_running == task_idle)
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
