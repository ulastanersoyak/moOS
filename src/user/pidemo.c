#include "lib/moos.h"

// priority inversion: low takes a lock, high waits for it, and mid, which
// doesnt need the lock at all, would keep low (and so high) from running.
// with priority inheritance low runs at high's priority while it holds the
// lock and mid has to wait instead. compare "pidemo" and "pidemo nopi"

#define LOCK_NAME "pidemo"
#define LOW_WORK_MS 200
#define MID_WORK_MS 400

static void
low (int lock, uint32_t loops_per_ms)
{
  mutex_lock (lock);
  printf ("low: got the lock, working for %u ms\n", LOW_WORK_MS);
  busy_ms (LOW_WORK_MS, loops_per_ms);
  printf ("low: unlocks\n");
  mutex_unlock (lock);
}

static void
mid (uint32_t loops_per_ms)
{
  printf ("mid: running for %u ms without the lock\n", MID_WORK_MS);
  busy_ms (MID_WORK_MS, loops_per_ms);
  printf ("mid: done\n");
}

static void
high (int lock)
{
  printf ("high: waiting for the lock\n");
  uint32_t start = uptime ();
  mutex_lock (lock);
  printf ("high: got the lock after %u ms\n", uptime () - start);
  mutex_unlock (lock);
}

static int
start (const char *role, int priority, char *loops)
{
  char *argv[] = { "pidemo", (char *)role, loops, 0 };
  struct run_opts opts
      = { .stdin = -1, .stdout = -1, .priority = priority, .background = 0 };
  int pid = run ("pidemo", argv, &opts);
  if (pid < 0)
    {
      print_error ("pidemo", pid);
    }
  return pid;
}

int
main (int argc, char **argv)
{
  if (argc == 3)
    {
      int lock = mutex_open (LOCK_NAME, 0);
      uint32_t loops_per_ms = atoi (argv[2]);
      if (strcmp (argv[1], "low") == 0)
        {
          low (lock, loops_per_ms);
        }
      else if (strcmp (argv[1], "mid") == 0)
        {
          mid (loops_per_ms);
        }
      else
        {
          high (lock);
        }
      return 0;
    }
  if (argc > 2 || (argc == 2 && strcmp (argv[1], "nopi") != 0))
    {
      eprint ("usage: pidemo [nopi]\n");
      return 1;
    }

  // the demo needs the real time policy, the parent sits above the three
  // tasks so it can start them in order
  int inherit = argc == 1;
  int old_policy = sched (SCHED_GET);
  sched (SCHED_RT);
  setprio (0, 25);
  char loops[12];
  snprintf (loops, sizeof (loops), "%u", busy_calibrate ());
  // created here so the children get it with the right flags
  int lock = mutex_open (LOCK_NAME, inherit ? MUTEX_INHERIT : 0);
  printf ("pidemo: priority inheritance %s\n", inherit ? "on" : "off");

  int pids[3];
  pids[0] = start ("low", 5, loops);
  // let low take the lock first
  sleep (50);
  pids[1] = start ("high", 20, loops);
  pids[2] = start ("mid", 10, loops);
  for (int i = 0; i < 3; i++)
    {
      if (pids[i] >= 0)
        {
          wait (pids[i]);
        }
    }
  mutex_close (lock);
  sched (old_policy);
  return 0;
}
