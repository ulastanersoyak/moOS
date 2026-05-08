#include "sh.h"

// the moOS shell. builtins change the shell itself or ask the kernel about
// the system, everything else is a program in BIN_DIR started with run

#define SH_MAX_ARGS 16
#define SH_MAX_STAGES 8
#define SH_MAX_DIR_ENTRIES 64
#define SH_MAX_TASKS 16
#define BIN_DIR "0:/bin/"
#define BIN_EXT ".elf"
// exit code of a task killed by an exception, ctrl+c or kill
#define TASK_KILLED (-1)

struct sh_command
{
  const char *name;
  const char *usage;
  void (*run) (int argc, char **argv);
};

static const struct sh_command commands[];

// one program of a pipeline
struct sh_stage
{
  int argc;
  char *argv[SH_MAX_ARGS + 1];
};

// "a < in | b | c > out &"
struct sh_pipeline
{
  struct sh_stage stages[SH_MAX_STAGES];
  int count;
  char *input;  // < file for the first stage
  char *output; // > or >> file for the last stage
  uint8_t append;
  uint8_t background;
};

// splits line into words. | < > >> and & are words of their own even without
// spaces around them, "double quotes" keep spaces in a word. words are copied
// to buf, which must hold twice the line. returns number of words or -1
static int
sh_tokenize (const char *line, char *buf, char **words, int max)
{
  int count = 0;
  while (*line)
    {
      if (*line == ' ')
        {
          line++;
          continue;
        }
      if (count == max)
        {
          return -1;
        }
      words[count++] = buf;
      if (*line == '|' || *line == '<' || *line == '&' || *line == '>')
        {
          *buf++ = *line;
          if (line[0] == '>' && line[1] == '>')
            {
              *buf++ = *++line;
            }
          line++;
        }
      else
        {
          uint8_t quoted = 0;
          while (*line && (quoted || !strchr (" |<>&", *line)))
            {
              if (*line == '"')
                {
                  quoted = !quoted;
                }
              else
                {
                  *buf++ = *line;
                }
              line++;
            }
        }
      *buf++ = '\0';
    }
  return count;
}

// returns 0 or prints what is wrong with the command line
static int
sh_parse (char **words, int count, struct sh_pipeline *pipeline)
{
  memset (pipeline, 0, sizeof (*pipeline));
  struct sh_stage *stage = &pipeline->stages[0];
  pipeline->count = 1;
  for (int i = 0; i < count; i++)
    {
      char *word = words[i];
      if (strcmp (word, "|") == 0)
        {
          if (stage->argc == 0 || pipeline->output
              || pipeline->count == SH_MAX_STAGES)
            {
              printf ("invalid pipe\n");
              return -1;
            }
          stage = &pipeline->stages[pipeline->count++];
        }
      else if (strcmp (word, "<") == 0 || strcmp (word, ">") == 0
               || strcmp (word, ">>") == 0)
        {
          if (i + 1 == count || strchr ("|<>&", words[i + 1][0]))
            {
              printf ("%s: missing file name\n", word);
              return -1;
            }
          if (word[0] == '<')
            {
              if (pipeline->count > 1)
                {
                  printf ("<: only the first program can read a file\n");
                  return -1;
                }
              pipeline->input = words[++i];
            }
          else
            {
              pipeline->output = words[++i];
              pipeline->append = word[1] == '>';
            }
        }
      else if (strcmp (word, "&") == 0)
        {
          if (i + 1 != count)
            {
              printf ("&: must be at the end of the line\n");
              return -1;
            }
          pipeline->background = 1;
        }
      else if (stage->argc == SH_MAX_ARGS)
        {
          printf ("too many arguments\n");
          return -1;
        }
      else
        {
          stage->argv[stage->argc++] = word;
        }
    }
  if (stage->argc == 0)
    {
      printf ("invalid pipe\n");
      return -1;
    }
  return 0;
}

static void
sh_print_error (const char *what, int err)
{
  setcolor (LIGHT_RED);
  print_error (what, err);
  setcolor (WHITE);
}

static void
cmd_help (int argc, char **argv)
{
  printf ("commands:\n");
  for (int i = 0; commands[i].name; i++)
    {
      printf ("  %s\n", commands[i].usage);
    }
  printf ("programs in %s run by name: ls, echo, cat, grep, sort, wc, cp,\n"
          "date, time, cowsay, moose, ... connect them with |, read a file\n"
          "with < file, write with > file or >> file, run in the background\n"
          "with &. ctrl+c stops the foreground programs, ctrl+d ends input\n"
          "typed into a program. tab completes, up and down go through the\n"
          "history\n",
          BIN_DIR);
}

static void
cmd_clear (int argc, char **argv)
{
  clear ();
}

static void
cmd_pwd (int argc, char **argv)
{
  char cwd[SH_LINE_MAX];
  getcwd (cwd, sizeof (cwd));
  printf ("%s\n", cwd);
}

static void
cmd_cd (int argc, char **argv)
{
  const char *dir = argc > 1 ? argv[1] : "/";
  int res = chdir (dir);
  if (res < 0)
    {
      sh_print_error (dir, res);
    }
}

// runs op on every path argument and reports failures
static void
sh_for_each_path (int argc, char **argv, const char *usage,
                  int (*op) (const char *path))
{
  if (argc < 2)
    {
      printf ("usage: %s\n", usage);
      return;
    }
  for (int i = 1; i < argc; i++)
    {
      int res = op (argv[i]);
      if (res < 0)
        {
          sh_print_error (argv[i], res);
        }
    }
}

static int
sh_touch (const char *path)
{
  // append mode creates missing files and keeps existing content
  int fd = open (path, O_APPEND);
  if (fd < 0)
    {
      return fd;
    }
  close (fd);
  return 0;
}

static void
cmd_touch (int argc, char **argv)
{
  sh_for_each_path (argc, argv, "touch <file>...", sh_touch);
}

static void
cmd_rm (int argc, char **argv)
{
  sh_for_each_path (argc, argv, "rm <file|empty dir>...", remove);
}

static void
cmd_mkdir (int argc, char **argv)
{
  sh_for_each_path (argc, argv, "mkdir <dir>...", mkdir);
}

static void
cmd_mem (int argc, char **argv)
{
  struct sysinfo info;
  sysinfo (&info);
  printf ("heap: %u kb used / %u kb total (%u of %u blocks)\n",
          info.heap_used_kb, info.heap_total_kb, info.heap_used_blocks,
          info.heap_total_blocks);
}

static void
cmd_uptime (int argc, char **argv)
{
  struct sysinfo info;
  sysinfo (&info);
  uint32_t ms = info.uptime_ms;
  printf ("up %u.%u%u s (%u ticks)\n", ms / 1000, (ms / 100) % 10,
          (ms / 10) % 10, info.ticks);
}

static void
cmd_sleep (int argc, char **argv)
{
  if (argc < 2)
    {
      printf ("usage: sleep <ms>\n");
      return;
    }
  sleep (atoi (argv[1]));
}

// "hello" becomes 0:/bin/hello.elf, names with a / or : are paths. returns
// 0 if there is no such file
static int
sh_find_program (const char *name, char *path, uint32_t max)
{
  if (strchr (name, '/') || strchr (name, ':'))
    {
      if (strlen (name) >= max)
        {
          return 0;
        }
      strcpy (path, name);
    }
  else
    {
      if (snprintf (path, max, "%s%s%s", BIN_DIR, name, BIN_EXT) >= (int)max)
        {
          return 0;
        }
    }
  int fd = open (path, O_READ);
  if (fd < 0)
    {
      return 0;
    }
  close (fd);
  return 1;
}

// opens a < or > file, prints an error and returns a negative error on
// failure
static int
sh_open (const char *name, int mode)
{
  int fd = open (name, mode);
  if (fd < 0)
    {
      sh_print_error (name, fd);
    }
  return fd;
}

// starts every program of the pipeline, connected by pipes. background
// pipelines keep running while the shell reads the next command, foreground
// ones get the keyboard until they all exit
static void
sh_run_pipeline (struct sh_pipeline *pipeline, int priority)
{
  char path[SH_LINE_MAX];
  for (int i = 0; i < pipeline->count; i++)
    {
      char *name = pipeline->stages[i].argv[0];
      if (!sh_find_program (name, path, sizeof (path)))
        {
          printf ("%s: unknown command, try help\n", name);
          return;
        }
    }

  int input = -1;
  int output = -1;
  if (pipeline->input && (input = sh_open (pipeline->input, O_READ)) < 0)
    {
      return;
    }
  if (pipeline->output
      && (output = sh_open (pipeline->output,
                            pipeline->append ? O_APPEND : O_WRITE))
             < 0)
    {
      if (input >= 0)
        {
          close (input);
        }
      return;
    }

  int pids[SH_MAX_STAGES];
  int next_input = input;
  for (int i = 0; i < pipeline->count; i++)
    {
      struct run_opts opts = { .stdin = next_input,
                               .stdout = output,
                               .priority = priority,
                               .background = pipeline->background,
                               .stderr = -1 };
      int pipe_fds[2];
      next_input = -1;
      // a failed pipe leaves the stage writing to the terminal
      if (i + 1 < pipeline->count && pipe (pipe_fds) == 0)
        {
          next_input = pipe_fds[0];
          opts.stdout = pipe_fds[1];
        }
      struct sh_stage *stage = &pipeline->stages[i];
      stage->argv[stage->argc] = 0;
      pids[i] = run (stage->argv[0], stage->argv, &opts);
      if (pids[i] < 0)
        {
          sh_print_error (stage->argv[0], pids[i]);
        }
      // the programs hold their own references now
      if (opts.stdin >= 0)
        {
          close (opts.stdin);
        }
      if (opts.stdout >= 0 && opts.stdout != output)
        {
          close (opts.stdout);
        }
    }
  if (output >= 0)
    {
      close (output);
    }

  if (pipeline->background)
    {
      printf ("[");
      for (int i = 0; i < pipeline->count; i++)
        {
          printf (i ? " %d" : "%d", pids[i]);
        }
      printf ("] started\n");
      return;
    }
  uint8_t killed = 0;
  int exit_code = 0;
  for (int i = 0; i < pipeline->count; i++)
    {
      if (pids[i] >= 0)
        {
          exit_code = wait (pids[i]);
          killed |= exit_code == TASK_KILLED;
        }
    }
  if (killed)
    {
      setcolor (LIGHT_RED);
      printf ("[killed]\n");
      setcolor (WHITE);
    }
  else if (exit_code != 0)
    {
      printf ("[exit code %d]\n", exit_code);
    }
}

// "run" and "prio" are prefixes of a command line, handled in
// sh_execute_line. these only run when they are used without a program
static void
cmd_run (int argc, char **argv)
{
  printf ("usage: run <program> [args] [| ...] [< in] [> out] [&]\n");
}

static void
cmd_prio (int argc, char **argv)
{
  printf ("usage: prio <%d-%d> <program> [args] [| ...] [&]\n", PRIO_MIN,
          PRIO_MAX);
}

static void
cmd_ps (int argc, char **argv)
{
  struct task_info list[SH_MAX_TASKS];
  int count = tasks (list, SH_MAX_TASKS);
  printf ("pid  name        state     prio  cpu\n");
  for (int i = 0; i < count; i++)
    {
      struct task_info *task = &list[i];
      uint32_t ms = task->cpu_ms;
      printf ("%-5d%-12s%-10s%-6u%u.%u%u s%s", task->pid, task->name,
              task->state, task->priority, ms / 1000, (ms / 100) % 10,
              (ms / 10) % 10, task->background ? " &" : "");
      if (task->period_ms)
        {
          printf ("  every %u ms, %u of %u late", task->period_ms,
                  task->deadline_misses, task->jobs);
        }
      printf ("\n");
    }
}

// kills by pid, or every task with the given name
static void
cmd_kill (int argc, char **argv)
{
  if (argc < 2)
    {
      printf ("usage: kill <pid|name>\n");
      return;
    }
  if (isdigit (argv[1][0]))
    {
      if (kill (atoi (argv[1])) < 0)
        {
          printf ("kill: no user task with pid %s\n", argv[1]);
        }
      return;
    }
  struct task_info list[SH_MAX_TASKS];
  int count = tasks (list, SH_MAX_TASKS);
  int killed = 0;
  for (int i = 0; i < count; i++)
    {
      if (strcmp (list[i].name, argv[1]) == 0 && kill (list[i].pid) == 0)
        {
          killed++;
        }
    }
  if (!killed)
    {
      printf ("kill: no user task named %s\n", argv[1]);
    }
}

static const char *
sh_policy_name (int policy)
{
  switch (policy)
    {
    case SCHED_RT:
      return "real time (fixed priority preemptive)";
    case SCHED_EDF:
      return "earliest deadline first";
    default:
      return "round robin";
    }
}

static void
cmd_sched (int argc, char **argv)
{
  int policy = SCHED_GET;
  if (argc > 1)
    {
      if (strcmp (argv[1], "rr") == 0)
        {
          policy = SCHED_RR;
        }
      else if (strcmp (argv[1], "rt") == 0)
        {
          policy = SCHED_RT;
        }
      else if (strcmp (argv[1], "edf") == 0)
        {
          policy = SCHED_EDF;
        }
      else
        {
          printf ("usage: sched [rr|rt|edf]\n");
          return;
        }
    }
  printf ("scheduler: %s\n", sh_policy_name (sched (policy)));
}

static void
cmd_reboot (int argc, char **argv)
{
  reboot ();
}

static void
cmd_exit (int argc, char **argv)
{
  exit (argc > 1 ? atoi (argv[1]) : 0);
}

static const struct sh_command commands[] = {
  { "help", "help", cmd_help },
  { "clear", "clear", cmd_clear },
  { "cd", "cd [dir]", cmd_cd },
  { "pwd", "pwd", cmd_pwd },
  { "touch", "touch <file>...", cmd_touch },
  { "rm", "rm <path>...", cmd_rm },
  { "mkdir", "mkdir <dir>...", cmd_mkdir },
  { "run", "run <program> ...", cmd_run },
  { "ps", "ps", cmd_ps },
  { "kill", "kill <pid|name>", cmd_kill },
  { "sched", "sched [rr|rt|edf]", cmd_sched },
  { "prio", "prio <n> <program> ...", cmd_prio },
  { "mem", "mem", cmd_mem },
  { "uptime", "uptime", cmd_uptime },
  { "sleep", "sleep <ms>", cmd_sleep },
  { "reboot", "reboot", cmd_reboot },
  { "exit", "exit [code]", cmd_exit },
  { 0, 0, 0 },
};

static void
sh_execute_line (char *line)
{
  char buf[2 * SH_LINE_MAX];
  char *word_table[SH_LINE_MAX];
  char **words = word_table;
  int count = sh_tokenize (line, buf, words, SH_LINE_MAX);
  if (count <= 0)
    {
      return;
    }

  // prefixes: "run <program> ..." skips the builtins, "prio <n> ..." sets
  // the real time priority
  int priority = PRIO_DEFAULT;
  uint8_t force_program = 0;
  while (count > 1
         && (strcmp (words[0], "run") == 0 || strcmp (words[0], "prio") == 0))
    {
      if (words[0][0] == 'r')
        {
          force_program = 1;
          words++;
          count--;
          continue;
        }
      int value = atoi (words[1]);
      if (count < 3 || !isdigit (words[1][0]) || value < PRIO_MIN
          || value > PRIO_MAX)
        {
          cmd_prio (0, 0);
          return;
        }
      priority = value;
      force_program = 1;
      words += 2;
      count -= 2;
    }

  struct sh_pipeline pipeline;
  if (sh_parse (words, count, &pipeline) < 0)
    {
      return;
    }
  struct sh_stage *first = &pipeline.stages[0];
  for (int i = 0; commands[i].name && !force_program; i++)
    {
      if (strcmp (first->argv[0], commands[i].name) == 0)
        {
          if (pipeline.count > 1 || pipeline.input || pipeline.output
              || pipeline.background)
            {
              printf ("%s: pipes, redirection and & only work with "
                      "programs\n",
                      first->argv[0]);
              return;
            }
          commands[i].run (first->argc, first->argv);
          return;
        }
    }
  sh_run_pipeline (&pipeline, priority);
}

// directories get a / so completion can go on inside them
static void
sh_completion_name (const char *name, uint8_t is_dir, char *out)
{
  uint32_t i = 0;
  for (; name[i] && i < SH_COMPLETION_LEN - 2; i++)
    {
      out[i] = name[i];
    }
  if (is_dir)
    {
      out[i++] = '/';
    }
  out[i] = '\0';
}

static int
sh_add_completion (const char *name, const char *typed, uint32_t typed_len,
                   char out[][SH_COMPLETION_LEN], int total, int max)
{
  if (total < max && strncasecmp (name, typed, typed_len) == 0)
    {
      strcpy (out[total++], name);
    }
  return total;
}

int
sh_complete (const char *prefix, uint32_t prefix_len, uint8_t is_command,
             uint32_t *name_start, char out[][SH_COMPLETION_LEN], int max)
{
  char word[SH_LINE_MAX];
  if (prefix_len >= sizeof (word))
    {
      return 0;
    }
  memcpy (word, prefix, prefix_len);
  word[prefix_len] = '\0';
  // static so the shell doesnt grow its heap while it runs
  static struct dirent entries[SH_MAX_DIR_ENTRIES];
  int total = 0;
  char name[SH_COMPLETION_LEN];
  if (is_command && !strchr (word, '/'))
    {
      // builtins and programs in the bin directory without their extension
      *name_start = 0;
      for (int i = 0; commands[i].name; i++)
        {
          total = sh_add_completion (commands[i].name, word, prefix_len, out,
                                     total, max);
        }
      int count = listdir (BIN_DIR, entries, SH_MAX_DIR_ENTRIES);
      for (int i = 0; i < count; i++)
        {
          char *dot = strchr (entries[i].name, '.');
          if (dot && strncasecmp (dot, BIN_EXT, 5) == 0)
            {
              *dot = '\0';
              sh_completion_name (entries[i].name, 0, name);
              total = sh_add_completion (name, word, prefix_len, out, total,
                                         max);
            }
        }
    }
  else
    {
      // entries of the directory part of the word, e.g. "dir/" in "dir/lo"
      uint32_t slash = 0;
      for (uint32_t i = 0; i < prefix_len; i++)
        {
          if (word[i] == '/')
            {
              slash = i + 1;
            }
        }
      *name_start = slash;
      char dir[SH_LINE_MAX];
      memcpy (dir, word, slash);
      dir[slash] = '\0';
      int count = listdir (dir, entries, SH_MAX_DIR_ENTRIES);
      for (int i = 0; i < count; i++)
        {
          sh_completion_name (entries[i].name, entries[i].is_dir, name);
          total = sh_add_completion (name, word + slash, prefix_len - slash,
                                     out, total, max);
        }
    }
  return total;
}

void
sh_print_prompt (void)
{
  char cwd[SH_LINE_MAX];
  getcwd (cwd, sizeof (cwd));
  setcolor (LIGHT_GREEN);
  printf ("moos %s", cwd);
  setcolor (WHITE);
  printf ("> ");
}

int
main (int argc, char **argv)
{
  char line[SH_LINE_MAX];
  printf ("\nwelcome to moOS! type help for commands\n");
  while (1)
    {
      sh_print_prompt ();
      sh_readline (line, sizeof (line));
      sh_execute_line (line);
    }
}
