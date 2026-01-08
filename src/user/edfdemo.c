#include "lib/moos.h"

// two periodic tasks that together need 94% of the cpu:
//   fast: a 30 ms job every 60 ms
//   slow: a 40 ms job every 90 ms
// under sched rt they get rate monotonic priorities (the shorter period
// wins), which cant schedule this set: fast takes 60 of the first 90 ms and
// slow misses its deadline every 180 ms. earliest deadline first keeps every
// deadline because the set needs less than 100% of the cpu

#define RUN_MS 1800

struct periodic
{
  const char *name;
  uint32_t period_ms;
  uint32_t work_ms;
  int priority; // rate monotonic, used by sched rt
};

static const struct periodic tasks_set[] = {
  { "fast", 60, 30, 20 },
  { "slow", 90, 40, 15 },
};

#define TASK_COUNT (sizeof (tasks_set) / sizeof (tasks_set[0]))

static void
periodic_main (const struct periodic *task, uint32_t loops_per_ms)
{
  uint32_t jobs = RUN_MS / task->period_ms;
  uint32_t missed = 0;
  period (task->period_ms);
  for (uint32_t i = 0; i < jobs; i++)
    {
      busy_ms (task->work_ms, loops_per_ms);
      missed += wait_period ();
    }
  period (0);
  printf ("%s: %u ms every %u ms, %u of %u deadlines missed\n", task->name,
          task->work_ms, task->period_ms, missed, jobs);
}

static const char *
policy_name (int policy)
{
  switch (policy)
    {
    case SCHED_RT:
      return "rt, rate monotonic priorities";
    case SCHED_EDF:
      return "edf";
    default:
      return "rr, priorities are ignored";
    }
}

int
main (int argc, char **argv)
{
  if (argc == 3)
    {
      uint32_t idx = atoi (argv[1]);
      if (idx < TASK_COUNT)
        {
          periodic_main (&tasks_set[idx], atoi (argv[2]));
        }
      return 0;
    }

  // the parent sits above both tasks so it can start them at the same time
  setprio (0, 25);
  char loops[12];
  snprintf (loops, sizeof (loops), "%u", busy_calibrate ());
  printf ("edfdemo: scheduler %s\n", policy_name (sched (SCHED_GET)));

  int pids[TASK_COUNT];
  for (uint32_t i = 0; i < TASK_COUNT; i++)
    {
      char idx[4];
      snprintf (idx, sizeof (idx), "%u", i);
      char *child_argv[] = { "edfdemo", idx, loops, 0 };
      struct run_opts opts = { .stdin = -1,
                               .stdout = -1,
                               .priority = tasks_set[i].priority,
                               .background = 0 };
      pids[i] = run ("edfdemo", child_argv, &opts);
      if (pids[i] < 0)
        {
          print_error ("edfdemo", pids[i]);
        }
    }
  for (uint32_t i = 0; i < TASK_COUNT; i++)
    {
      if (pids[i] >= 0)
        {
          wait (pids[i]);
        }
    }
  return 0;
}
