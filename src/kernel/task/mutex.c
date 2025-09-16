#include "mutex.h"
#include "../../libc/string/string.h"
#include "../config.h"
#include "sched.h"

// asm functions
uint32_t irq_save (void);
void irq_restore (uint32_t eflags);

struct mutex
{
  char name[MUTEX_NAME_LEN];
  uint32_t refs; // tasks that have it open, 0 for a free slot
  uint8_t inherit;
  struct task *owner;
};

static struct mutex mutexes[MAX_MUTEXES];

// mutex id of the running task or 0 if it didnt open it
static struct mutex *
mutex_get (int32_t id)
{
  if (id < 0 || id >= MAX_MUTEXES || !mutexes[id].refs
      || !(task_running->mutexes & (1u << id)))
    {
      return 0;
    }
  return &mutexes[id];
}

int32_t
mutex_open (const char *name, uint32_t flags)
{
  if (strlen (name) >= MUTEX_NAME_LEN)
    {
      return -INVALID_ARG_ERROR;
    }
  uint32_t eflags = irq_save ();
  int32_t free_id = -1;
  int32_t id = -1;
  for (int32_t i = 0; i < MAX_MUTEXES; i++)
    {
      if (mutexes[i].refs && strcmp (mutexes[i].name, name) == 0)
        {
          id = i;
          break;
        }
      if (!mutexes[i].refs && free_id < 0)
        {
          free_id = i;
        }
    }
  if (id < 0 && free_id >= 0)
    {
      id = free_id;
      memset (&mutexes[id], 0, sizeof (struct mutex));
      strcpy (mutexes[id].name, name);
      mutexes[id].inherit = (flags & MUTEX_INHERIT) != 0;
    }
  if (id >= 0 && !(task_running->mutexes & (1u << id)))
    {
      mutexes[id].refs++;
      task_running->mutexes |= 1u << id;
    }
  irq_restore (eflags);
  return id >= 0 ? id : -NO_MEMORY_ERROR;
}

// raises the priority of owner, and of the owners it waits for in turn,
// to at least priority
static void
mutex_boost (struct task *owner, uint8_t priority)
{
  // the chain of owners is short, the depth limit only guards against
  // deadlocked tasks waiting on each other
  for (int depth = 0; owner && depth < MAX_TASKS; depth++)
    {
      if (owner->priority >= priority)
        {
          return;
        }
      owner->priority = priority;
      struct mutex *next = owner->wait_mutex;
      owner = next && next->inherit ? next->owner : 0;
    }
}

void
mutex_update_priority (struct task *task)
{
  // killed tasks keep the max priority so they get to exit
  if (task->killed)
    {
      return;
    }
  uint8_t priority = task->base_priority;
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *waiter = &task_table[i];
      struct mutex *mutex = waiter->wait_mutex;
      if (waiter->state == TASK_BLOCKED && mutex && mutex->owner == task
          && mutex->inherit && waiter->priority > priority)
        {
          priority = waiter->priority;
        }
    }
  task->priority = priority;
  // a lower priority can mean that another task should run now
  sched_request_resched ();
}

int32_t
mutex_lock (int32_t id)
{
  uint32_t eflags = irq_save ();
  struct mutex *mutex = mutex_get (id);
  if (!mutex || mutex->owner == task_running)
    {
      irq_restore (eflags);
      return -INVALID_ARG_ERROR;
    }
  while (mutex->owner && mutex->owner != task_running
         && !task_running->killed)
    {
      task_running->wait_mutex = mutex;
      if (mutex->inherit)
        {
          mutex_boost (mutex->owner, task_running->priority);
        }
      task_block_on (mutex);
      task_running->wait_mutex = 0;
    }
  int32_t res = OK;
  if (!mutex->owner)
    {
      mutex->owner = task_running;
    }
  else if (mutex->owner != task_running)
    {
      // killed while waiting, the owner doesnt need our priority anymore
      mutex_update_priority (mutex->owner);
      res = TASK_KILLED;
    }
  irq_restore (eflags);
  return res;
}

// gives the mutex to its highest priority waiter, or frees it
static void
mutex_hand_over (struct mutex *mutex)
{
  struct task *old_owner = mutex->owner;
  struct task *next = 0;
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->state == TASK_BLOCKED && task->wait_channel == mutex
          && (!next || task->priority > next->priority))
        {
          next = task;
        }
    }
  mutex->owner = next;
  if (next)
    {
      sched_wake (next);
    }
  mutex_update_priority (old_owner);
}

int32_t
mutex_unlock (int32_t id)
{
  uint32_t eflags = irq_save ();
  struct mutex *mutex = mutex_get (id);
  if (!mutex || mutex->owner != task_running)
    {
      irq_restore (eflags);
      return -INVALID_ARG_ERROR;
    }
  mutex_hand_over (mutex);
  irq_restore (eflags);
  return OK;
}

// interrupts must be disabled
static void
mutex_close_task (struct task *task, int32_t id)
{
  struct mutex *mutex = &mutexes[id];
  if (mutex->owner == task)
    {
      mutex_hand_over (mutex);
    }
  task->mutexes &= ~(1u << id);
  mutex->refs--;
}

int32_t
mutex_close (int32_t id)
{
  uint32_t eflags = irq_save ();
  if (!mutex_get (id))
    {
      irq_restore (eflags);
      return -INVALID_ARG_ERROR;
    }
  mutex_close_task (task_running, id);
  irq_restore (eflags);
  return OK;
}

void
mutex_release_all (struct task *task)
{
  uint32_t eflags = irq_save ();
  for (int32_t id = 0; id < MAX_MUTEXES; id++)
    {
      if (task->mutexes & (1u << id))
        {
          mutex_close_task (task, id);
        }
    }
  irq_restore (eflags);
}
