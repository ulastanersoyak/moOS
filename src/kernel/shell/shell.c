#include "shell.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"
#include "../../drivers/timer/pit.h"
#include "../../libc/ctype/ctype.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/stdlib/stdlib.h"
#include "../../libc/string/string.h"
#include "../ascii/ascii.h"
#include "../config.h"
#include "../io/io.h"
#include "../kmem/kheap.h"
#include "../task/handle.h"
#include "../task/sched.h"
#include "../task/task.h"
#include "shell_internal.h"

#define SHELL_MAX_ARGS TASK_MAX_ARGS
#define SHELL_MAX_DIR_ENTRIES 64

struct shell_command
{
  const char *name;
  const char *usage;
  void (*run) (int argc, char **argv);
};

static const struct shell_command commands[];

#define SHELL_MAX_STAGES 8

// one program of a pipeline
struct shell_stage
{
  int argc;
  char *argv[SHELL_MAX_ARGS + 1];
};

// "a < in | b | c > out &"
struct shell_pipeline
{
  struct shell_stage stages[SHELL_MAX_STAGES];
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
shell_tokenize (const char *line, char *buf, char **words, int max)
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
shell_parse (char **words, int count, struct shell_pipeline *pipeline)
{
  memset (pipeline, 0, sizeof (*pipeline));
  struct shell_stage *stage = &pipeline->stages[0];
  pipeline->count = 1;
  for (int i = 0; i < count; i++)
    {
      char *word = words[i];
      if (strcmp (word, "|") == 0)
        {
          if (stage->argc == 0 || pipeline->output
              || pipeline->count == SHELL_MAX_STAGES)
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
      else if (stage->argc == SHELL_MAX_ARGS)
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

// resolves a path relative to the working directory. returns 0 if too long
static int
shell_resolve_path (const char *arg, char *out)
{
  return task_resolve_path (arg, out);
}

static void
shell_print_error (const char *what, int32_t err)
{
  const char *reason = "error";
  switch (-err)
    {
    case NOT_FOUND_ERROR:
      reason = "not found";
      break;
    case NOT_DIRECTORY_ERROR:
      reason = "not a directory";
      break;
    case INVALID_PATH_ERROR:
      reason = "invalid path";
      break;
    case NO_MEMORY_ERROR:
      reason = "out of memory";
      break;
    case INVALID_ARG_ERROR:
      reason = "invalid file";
      break;
    case DIR_NOT_EMPTY_ERROR:
      reason = "directory not empty";
      break;
    case FILE_EXISTS_ERROR:
      reason = "already exists";
      break;
    case BROKEN_PIPE_ERROR:
      reason = "broken pipe";
      break;
    }
  printf ("%C%s: %s%C\n", light_red, what, reason, white);
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
          "date, time, ... connect them with |, read a file with < file,\n"
          "write with > file or >> file, run in the background with &.\n"
          "ctrl+c stops the foreground programs, ctrl+d ends input typed\n"
          "into a program. tab completes, up and down go through the "
          "history\n",
          TASK_BIN_DIR);
}

static void
cmd_clear (int argc, char **argv)
{
  terminal_clean ();
}

static void
cmd_pwd (int argc, char **argv)
{
  printf ("%s\n", task_cwd ());
}

static void
cmd_cd (int argc, char **argv)
{
  char path[MAX_PATH_LEN + 1];
  if (!shell_resolve_path (argc > 1 ? argv[1] : "/", path))
    {
      shell_print_error (argv[1], -INVALID_PATH_ERROR);
      return;
    }
  // listing fails for files and missing directories
  struct file_dirent entry;
  int res = listdir (path, &entry, 1);
  if (res < 0)
    {
      shell_print_error (path, res);
      return;
    }
  task_set_cwd (path);
}

// runs op on every path argument and reports failures
static void
shell_for_each_path (int argc, char **argv, const char *usage,
                     int (*op) (const char *path))
{
  if (argc < 2)
    {
      printf ("usage: %s\n", usage);
      return;
    }
  for (int i = 1; i < argc; i++)
    {
      char path[MAX_PATH_LEN + 1];
      int res = shell_resolve_path (argv[i], path) ? op (path)
                                                   : -INVALID_PATH_ERROR;
      if (res < 0)
        {
          shell_print_error (argv[i], res);
        }
    }
}

static int
shell_touch (const char *path)
{
  // append mode creates missing files and keeps existing content
  int fd = fopen (path, "a");
  if (!fd)
    {
      return -INVALID_PATH_ERROR;
    }
  fclose (fd);
  return OK;
}

static void
cmd_touch (int argc, char **argv)
{
  shell_for_each_path (argc, argv, "touch <file>...", shell_touch);
}

static void
cmd_rm (int argc, char **argv)
{
  shell_for_each_path (argc, argv, "rm <file|empty dir>...", remove);
}

static void
cmd_mkdir (int argc, char **argv)
{
  shell_for_each_path (argc, argv, "mkdir <dir>...", mkdir);
}

static void
cmd_mem (int argc, char **argv)
{
  struct heap_stats stats;
  kheap_stats (&stats);
  uint32_t block_kb = HEAP_BLOCK_SIZE / 1024;
  printf ("heap: %u kb used / %u kb total (%u of %u blocks)\n",
          stats.used_blocks * block_kb, stats.total_blocks * block_kb,
          stats.used_blocks, stats.total_blocks);
}

static void
cmd_uptime (int argc, char **argv)
{
  uint32_t ms = pit_uptime_ms ();
  printf ("up %u.%u%u s (%u ticks)\n", ms / 1000, (ms / 100) % 10,
          (ms / 10) % 10, pit_ticks ());
}

static void
cmd_sleep (int argc, char **argv)
{
  if (argc < 2)
    {
      printf ("usage: sleep <ms>\n");
      return;
    }
  task_sleep_ms (atoi (argv[1]));
}

// rejoins args into one string for the ascii art commands
static void
shell_join_args (int argc, char **argv, char *out, uint32_t max)
{
  out[0] = '\0';
  for (int i = 1; i < argc; i++)
    {
      if (strlen (out) + strlen (argv[i]) + 2 > max)
        {
          break;
        }
      if (i > 1)
        {
          strcat (out, " ");
        }
      strcat (out, argv[i]);
    }
}

static void
cmd_cowsay (int argc, char **argv)
{
  char text[SHELL_LINE_MAX];
  shell_join_args (argc, argv, text, sizeof (text));
  cowsay (argc > 1 ? text : "moo", light_brown);
}

static void
cmd_moose (int argc, char **argv)
{
  char text[SHELL_LINE_MAX];
  shell_join_args (argc, argv, text, sizeof (text));
  moose (argc > 1 ? text : "moOS", light_brown);
}

// opens a < or > file as a handle, prints an error and returns 0 on failure
static struct handle *
shell_open (const char *name, const char *mode)
{
  char path[MAX_PATH_LEN + 1];
  int fd = shell_resolve_path (name, path) ? fopen (path, mode) : 0;
  struct handle *handle = fd ? handle_new_file (fd) : 0;
  if (!handle)
    {
      if (fd)
        {
          fclose (fd);
        }
      shell_print_error (name, -NOT_FOUND_ERROR);
    }
  return handle;
}

// starts every program of the pipeline, connected by pipes. background
// pipelines keep running while the shell reads the next command, foreground
// ones get the keyboard until they all exit
static void
shell_run_pipeline (struct shell_pipeline *pipeline, uint8_t priority)
{
  char paths[SHELL_MAX_STAGES][MAX_PATH_LEN + 1];
  for (int i = 0; i < pipeline->count; i++)
    {
      char *name = pipeline->stages[i].argv[0];
      if (!task_find_program (name, paths[i]))
        {
          printf ("%s: unknown command, try help\n", name);
          return;
        }
    }

  struct handle *input = 0;
  struct handle *output = 0;
  if (pipeline->input && !(input = shell_open (pipeline->input, "r")))
    {
      return;
    }
  if (pipeline->output
      && !(output
           = shell_open (pipeline->output, pipeline->append ? "a" : "w")))
    {
      if (input)
        {
          handle_close (input);
        }
      return;
    }

  int32_t pids[SHELL_MAX_STAGES];
  struct handle *next_input = input;
  for (int i = 0; i < pipeline->count; i++)
    {
      struct handle *stage_input = next_input;
      struct handle *stage_output = output;
      next_input = 0;
      if (i + 1 < pipeline->count)
        {
          // a failed pipe leaves the stage reading or writing the terminal
          handle_new_pipe (&next_input, &stage_output);
        }
      struct shell_stage *stage = &pipeline->stages[i];
      pids[i] = task_spawn (paths[i], stage->argc, stage->argv, priority,
                            pipeline->background, stage_input, stage_output);
      if (pids[i] < 0)
        {
          shell_print_error (paths[i], pids[i]);
        }
      // the tasks hold their own references now
      if (stage_input)
        {
          handle_close (stage_input);
        }
      if (stage_output && stage_output != output)
        {
          handle_close (stage_output);
        }
    }
  if (output)
    {
      handle_close (output);
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
  int32_t exit_code = 0;
  for (int i = 0; i < pipeline->count; i++)
    {
      if (pids[i] >= 0)
        {
          exit_code = task_wait (pids[i]);
          killed |= exit_code == TASK_KILLED;
        }
    }
  if (killed)
    {
      printf ("%C[killed]%C\n", light_red, white);
    }
  else if (exit_code != 0)
    {
      printf ("[exit code %d]\n", exit_code);
    }
}

// "run" and "prio" are prefixes of a command line, handled in
// shell_execute_line. these only run when they are used without a program
static void
cmd_run (int argc, char **argv)
{
  printf ("usage: run <program> [args] [| ...] [< in] [> out] [&]\n");
}

static void
cmd_prio (int argc, char **argv)
{
  printf ("usage: prio <1-%d> <program> [args] [| ...] [&]\n",
          TASK_PRIORITY_USER_MAX);
}

static void
shell_pad (uint32_t written, uint32_t width)
{
  for (; written < width; written++)
    {
      terminal_putchar (' ');
    }
}

static uint32_t
shell_num_len (uint32_t num)
{
  uint32_t len = 1;
  while (num >= 10)
    {
      num /= 10;
      len++;
    }
  return len;
}

static void
cmd_ps (int argc, char **argv)
{
  printf ("pid  name        state     prio  cpu\n");
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->state == TASK_UNUSED)
        {
          continue;
        }
      printf ("%u", task->pid);
      shell_pad (shell_num_len (task->pid), 5);
      printf ("%s", task->name);
      shell_pad (strlen (task->name), 12);
      printf ("%s", task_state_name (task->state));
      shell_pad (strlen (task_state_name (task->state)), 10);
      printf ("%u", task->priority);
      shell_pad (shell_num_len (task->priority), 6);
      printf ("%u.%u%u s%s\n", task->cpu_ticks / PIT_FREQUENCY_HZ,
              (task->cpu_ticks / 10) % 10, task->cpu_ticks % 10,
              task->background ? " &" : "");
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
      if (task_kill (atoi (argv[1])) < 0)
        {
          printf ("kill: no user task with pid %s\n", argv[1]);
        }
      return;
    }
  int killed = 0;
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->state != TASK_UNUSED && strcmp (task->name, argv[1]) == 0
          && task_kill (task->pid) == OK)
        {
          killed++;
        }
    }
  if (!killed)
    {
      printf ("kill: no user task named %s\n", argv[1]);
    }
}

static void
cmd_sched (int argc, char **argv)
{
  if (argc > 1)
    {
      if (strcmp (argv[1], "rr") == 0)
        {
          sched_set_policy (SCHED_ROUND_ROBIN);
        }
      else if (strcmp (argv[1], "rt") == 0)
        {
          sched_set_policy (SCHED_REAL_TIME);
        }
      else
        {
          printf ("usage: sched [rr|rt]\n");
          return;
        }
    }
  printf ("scheduler: %s\n", sched_policy_name (sched_get_policy ()));
}

static void
cmd_reboot (int argc, char **argv)
{
  // pulse the cpu reset line through the keyboard controller
  outb (0x64, 0xFE);
}

static const struct shell_command commands[] = {
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
  { "sched", "sched [rr|rt]", cmd_sched },
  { "prio", "prio <n> <program> ...", cmd_prio },
  { "mem", "mem", cmd_mem },
  { "uptime", "uptime", cmd_uptime },
  { "sleep", "sleep <ms>", cmd_sleep },
  { "cowsay", "cowsay <text>", cmd_cowsay },
  { "moose", "moose <text>", cmd_moose },
  { "reboot", "reboot", cmd_reboot },
  { 0, 0, 0 },
};

static void
shell_execute_line (char *line)
{
  char buf[2 * SHELL_LINE_MAX];
  char *word_table[SHELL_LINE_MAX];
  char **words = word_table;
  int count = shell_tokenize (line, buf, words, SHELL_LINE_MAX);
  if (count <= 0)
    {
      return;
    }

  // prefixes: "run <program> ..." skips the builtins, "prio <n> ..." sets
  // the real time priority
  uint8_t priority = TASK_PRIORITY_DEFAULT;
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
      uint32_t value = atoi (words[1]);
      if (count < 3 || !isdigit (words[1][0]) || value < 1
          || value > TASK_PRIORITY_USER_MAX)
        {
          cmd_prio (0, 0);
          return;
        }
      priority = value;
      force_program = 1;
      words += 2;
      count -= 2;
    }

  struct shell_pipeline pipeline;
  if (shell_parse (words, count, &pipeline) < 0)
    {
      return;
    }
  struct shell_stage *first = &pipeline.stages[0];
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
  shell_run_pipeline (&pipeline, priority);
}

// lowercases a name for completion, the file system ignores case anyway
static void
shell_completion_name (const char *name, uint8_t is_dir, char *out)
{
  uint32_t i = 0;
  for (; name[i] && i < SHELL_COMPLETION_LEN - 2; i++)
    {
      out[i] = tolower (name[i]);
    }
  if (is_dir)
    {
      out[i++] = '/';
    }
  out[i] = '\0';
}

static int
shell_add_completion (const char *name, const char *typed, uint32_t typed_len,
                      char out[][SHELL_COMPLETION_LEN], int total, int max)
{
  if (total < max && istrncmp (name, typed, typed_len) == 0)
    {
      strcpy (out[total++], name);
    }
  return total;
}

int
shell_complete (const char *prefix, uint32_t prefix_len, uint8_t is_command,
                uint32_t *name_start, char out[][SHELL_COMPLETION_LEN],
                int max)
{
  char word[MAX_PATH_LEN + 1];
  if (prefix_len > MAX_PATH_LEN)
    {
      return 0;
    }
  memcpy (word, prefix, prefix_len);
  word[prefix_len] = '\0';
  struct file_dirent *entries
      = calloc (sizeof (struct file_dirent) * SHELL_MAX_DIR_ENTRIES);
  if (!entries)
    {
      return 0;
    }

  int total = 0;
  char name[SHELL_COMPLETION_LEN];
  if (is_command && !strchr (word, '/'))
    {
      // builtins and programs in the bin directory without their extension
      *name_start = 0;
      for (int i = 0; commands[i].name; i++)
        {
          total = shell_add_completion (commands[i].name, word, prefix_len,
                                        out, total, max);
        }
      int count = listdir (TASK_BIN_DIR, entries, SHELL_MAX_DIR_ENTRIES);
      for (int i = 0; i < count; i++)
        {
          char *dot = strchr (entries[i].name, '.');
          if (dot && istrncmp (dot, TASK_BIN_EXT, 5) == 0)
            {
              *dot = '\0';
              shell_completion_name (entries[i].name, 0, name);
              total = shell_add_completion (name, word, prefix_len, out, total,
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
      char dir[MAX_PATH_LEN + 1];
      char dir_part[MAX_PATH_LEN + 1];
      memcpy (dir_part, word, slash);
      dir_part[slash] = '\0';
      if (shell_resolve_path (dir_part, dir))
        {
          int count = listdir (dir, entries, SHELL_MAX_DIR_ENTRIES);
          for (int i = 0; i < count; i++)
            {
              shell_completion_name (entries[i].name, entries[i].is_dir, name);
              total = shell_add_completion (
                  name, word + slash, prefix_len - slash, out, total, max);
            }
        }
    }
  free (entries);
  return total;
}

void
shell_print_prompt (void)
{
  printf ("%Cmoos %s%C> ", light_green, task_cwd (), white);
}

void
shell_run (void)
{
  char line[SHELL_LINE_MAX];
  printf ("\nwelcome to moOS! type help for commands\n");
  while (1)
    {
      shell_print_prompt ();
      shell_readline (line, sizeof (line));
      shell_execute_line (line);
    }
}
