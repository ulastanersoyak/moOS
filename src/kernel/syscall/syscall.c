#include "syscall.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../drivers/rtc/rtc.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"
#include "../../drivers/screen/vga_gfx.h"
#include "../../drivers/timer/pit.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"
#include "../config.h"
#include "../io/io.h"
#include "../kmem/kheap.h"
#include "../task/handle.h"
#include "../task/mutex.h"
#include "../task/sched.h"
#include "../task/task.h"

// longest string a task can print with one syscall
#define SYSCALL_MAX_PRINT 4096

#define STDIN_FD 0
#define STDOUT_FD 1

static int32_t
sys_write (int fd, const void *buf, uint32_t len)
{
  struct handle *handle = task_fd_get (fd);
  if (!handle || !task_user_range_valid (buf, len))
    {
      return -INVALID_ARG_ERROR;
    }
  return handle_write (handle, buf, len);
}

static int32_t
sys_read (int fd, void *buf, uint32_t len)
{
  struct handle *handle = task_fd_get (fd);
  if (!handle || !task_user_range_valid (buf, len))
    {
      return -INVALID_ARG_ERROR;
    }
  return handle_read (handle, buf, len);
}

// length of a user string, or a negative error if it isnt valid user memory
static int32_t
sys_user_strlen (const char *str, uint32_t max)
{
  // check every char before touching it, user pointers cant be trusted
  for (uint32_t len = 0; len <= max; len++)
    {
      if (!task_user_range_valid (str + len, 1))
        {
          return -INVALID_ARG_ERROR;
        }
      if (str[len] == '\0')
        {
          return len;
        }
    }
  return -INVALID_ARG_ERROR;
}

static int32_t
sys_print (const char *str)
{
  int32_t len = sys_user_strlen (str, SYSCALL_MAX_PRINT);
  return len < 0 ? len : sys_write (STDOUT_FD, str, len);
}

// copies a user string into out, which holds max chars plus the terminator
static int32_t
sys_copy_string (const char *user_str, char *out, uint32_t max)
{
  int32_t len = sys_user_strlen (user_str, max);
  if (len < 0)
    {
      return len;
    }
  memcpy (out, user_str, len + 1);
  return len;
}

// copies a path string from user memory and resolves it against the
// working directory of the task
static int32_t
sys_get_path (const char *user_path, char *out)
{
  char path[MAX_PATH_LEN + 1];
  if (sys_copy_string (user_path, path, MAX_PATH_LEN) < 0)
    {
      return -INVALID_PATH_ERROR;
    }
  return task_resolve_path (path, out) ? OK : -INVALID_PATH_ERROR;
}

static int32_t
sys_open (const char *user_path, uint32_t mode)
{
  static const char *modes[] = { "r", "w", "a" };
  char path[MAX_PATH_LEN + 1];
  int32_t res = sys_get_path (user_path, path);
  if (res < 0)
    {
      return res;
    }
  if (mode > 2)
    {
      return -INVALID_ARG_ERROR;
    }
  int file = fopen (path, modes[mode]);
  if (!file)
    {
      return -NOT_FOUND_ERROR;
    }
  struct handle *handle = handle_new_file (file);
  if (!handle)
    {
      fclose (file);
      return -NO_MEMORY_ERROR;
    }
  int fd = task_fd_install (handle);
  if (fd < 0)
    {
      handle_close (handle);
    }
  return fd;
}

// kernel fd of a file opened by the task, 0 for pipes and the terminal
static int
sys_file_fd (int fd)
{
  struct handle *handle = task_fd_get (fd);
  return handle && handle->type == HANDLE_FILE ? handle->file_fd : 0;
}

static int32_t
sys_filesize (int fd)
{
  int file = sys_file_fd (fd);
  struct file_stat stat;
  if (!file || fstat (file, &stat) != OK)
    {
      return -INVALID_ARG_ERROR;
    }
  return stat.filesize;
}

static int32_t
sys_seek (int fd, int32_t offset, uint32_t whence)
{
  int file = sys_file_fd (fd);
  return file ? fseek (file, offset, whence) : -INVALID_ARG_ERROR;
}

static int32_t
sys_listdir (const char *user_path, struct file_dirent *out, int max)
{
  char path[MAX_PATH_LEN + 1];
  int32_t res = sys_get_path (user_path, path);
  if (res < 0)
    {
      return res;
    }
  if (max < 0 || !task_user_range_valid (out, max * sizeof (*out)))
    {
      return -INVALID_ARG_ERROR;
    }
  return listdir (path, out, max);
}

static int32_t
sys_path_op (const char *user_path, int (*op) (const char *path))
{
  char path[MAX_PATH_LEN + 1];
  int32_t res = sys_get_path (user_path, path);
  return res < 0 ? res : op (path);
}

static int32_t
sys_pipe (int *fds)
{
  if (!task_user_range_valid (fds, 2 * sizeof (int)))
    {
      return -INVALID_ARG_ERROR;
    }
  struct handle *read_end;
  struct handle *write_end;
  int32_t res = handle_new_pipe (&read_end, &write_end);
  if (res < 0)
    {
      return res;
    }
  int read_fd = task_fd_install (read_end);
  int write_fd = read_fd < 0 ? read_fd : task_fd_install (write_end);
  if (write_fd < 0)
    {
      if (read_fd >= 0)
        {
          task_fd_close (read_fd);
        }
      else
        {
          handle_close (read_end);
        }
      handle_close (write_end);
      return write_fd;
    }
  fds[0] = read_fd;
  fds[1] = write_fd;
  return OK;
}

// how run starts a program, must match src/user/lib/moos.h
struct run_opts
{
  int32_t stdin; // fds of the caller, -1 passes on its own
  int32_t stdout;
  int32_t priority;
  int32_t background;
};

// starts a program with arguments copied out of user memory. stdin and
// stdout of the child are handles of the caller, 0 inherits them
static int32_t
sys_start (const char *user_name, char **user_argv, struct handle *stdin,
           struct handle *stdout, uint8_t priority, uint8_t background)
{
  char name[MAX_PATH_LEN + 1];
  char path[MAX_PATH_LEN + 1];
  if (sys_copy_string (user_name, name, MAX_PATH_LEN) < 0)
    {
      return -INVALID_PATH_ERROR;
    }
  if (!task_find_program (name, path))
    {
      return -NOT_FOUND_ERROR;
    }

  // argument strings are packed into one buffer, argv points into it
  static char args[TASK_MAX_ARGS_BYTES];
  char *argv[TASK_MAX_ARGS];
  int argc = 0;
  uint32_t used = 0;
  while (1)
    {
      if (argc > TASK_MAX_ARGS
          || !task_user_range_valid (&user_argv[argc], sizeof (char *)))
        {
          return -INVALID_ARG_ERROR;
        }
      if (!user_argv[argc])
        {
          break;
        }
      if (argc == TASK_MAX_ARGS)
        {
          return -INVALID_ARG_ERROR;
        }
      int32_t len = sys_copy_string (user_argv[argc], args + used,
                                     TASK_MAX_ARGS_BYTES - used - 1);
      if (len < 0)
        {
          return len;
        }
      argv[argc++] = args + used;
      used += len + 1;
    }
  return task_spawn (path, argc, argv, priority,
                     background ? TASK_SPAWN_BACKGROUND : 0, stdin, stdout);
}

// io holds the fds that become stdin and stdout of the child, 0 inherits
// both. the child gets the priority of the caller
static int32_t
sys_spawn (const char *user_name, char **user_argv, const int *io)
{
  struct handle *stdin = 0;
  struct handle *stdout = 0;
  if (io)
    {
      if (!task_user_range_valid (io, 2 * sizeof (int)))
        {
          return -INVALID_ARG_ERROR;
        }
      stdin = task_fd_get (io[0]);
      stdout = task_fd_get (io[1]);
      if (!stdin || !stdout)
        {
          return -INVALID_ARG_ERROR;
        }
    }
  return sys_start (user_name, user_argv, stdin, stdout,
                    task_running->priority, 0);
}

// like spawn, with a real time priority and optionally in the background
static int32_t
sys_run (const char *user_name, char **user_argv,
         const struct run_opts *user_opts)
{
  if (!task_user_range_valid (user_opts, sizeof (*user_opts)))
    {
      return -INVALID_ARG_ERROR;
    }
  struct run_opts opts = *user_opts;
  if (opts.priority < 1 || opts.priority > TASK_PRIORITY_USER_MAX)
    {
      return -INVALID_ARG_ERROR;
    }
  struct handle *stdin = opts.stdin < 0 ? 0 : task_fd_get (opts.stdin);
  struct handle *stdout = opts.stdout < 0 ? 0 : task_fd_get (opts.stdout);
  if ((opts.stdin >= 0 && !stdin) || (opts.stdout >= 0 && !stdout))
    {
      return -INVALID_ARG_ERROR;
    }
  return sys_start (user_name, user_argv, stdin, stdout, opts.priority,
                    opts.background != 0);
}

// changes the working directory, which must be an existing directory
static int32_t
sys_chdir (const char *user_path)
{
  char path[MAX_PATH_LEN + 1];
  int32_t res = sys_get_path (user_path, path);
  if (res < 0)
    {
      return res;
    }
  // listing fails for files and missing directories
  struct file_dirent entry;
  res = listdir (path, &entry, 1);
  if (res < 0)
    {
      return res;
    }
  task_set_cwd (path);
  return OK;
}

static int32_t
sys_getcwd (char *buf, uint32_t len)
{
  uint32_t cwd_len = strlen (task_cwd ());
  if (!task_user_range_valid (buf, len) || cwd_len + 1 > len)
    {
      return -INVALID_ARG_ERROR;
    }
  memcpy (buf, task_cwd (), cwd_len + 1);
  return cwd_len;
}

static int32_t
sys_tasks (struct task_info *out, int max)
{
  if (max < 0 || !task_user_range_valid (out, max * sizeof (*out)))
    {
      return -INVALID_ARG_ERROR;
    }
  return task_list (out, max);
}

// switches the scheduling policy unless policy is -1, returns the policy
static int32_t
sys_sched (int32_t policy)
{
  if (policy == SCHED_ROUND_ROBIN || policy == SCHED_REAL_TIME
      || policy == SCHED_EDF)
    {
      sched_set_policy (policy);
    }
  else if (policy != -1)
    {
      return -INVALID_ARG_ERROR;
    }
  return sched_get_policy ();
}

static int32_t
sys_setprio (int32_t pid, int32_t priority)
{
  if (priority < 1 || priority > TASK_PRIORITY_USER_MAX)
    {
      return -INVALID_ARG_ERROR;
    }
  return task_set_priority (pid, priority);
}

// terminal cursor for line editing, must match src/user/lib/moos.h
struct cursor
{
  uint32_t row;
  uint32_t column;
  uint32_t scrolls; // times the screen scrolled up since boot
};

static int32_t
sys_getcursor (struct cursor *out)
{
  if (!task_user_range_valid (out, sizeof (*out)))
    {
      return -INVALID_ARG_ERROR;
    }
  size_t row;
  size_t column;
  terminal_get_cursor (&row, &column);
  out->row = row;
  out->column = column;
  out->scrolls = terminal_scroll_count ();
  return OK;
}

static int32_t
sys_setcursor (uint32_t row, uint32_t column)
{
  if (row >= VGA_HEIGHT || column >= VGA_WIDTH)
    {
      return -INVALID_ARG_ERROR;
    }
  terminal_set_cursor (row, column);
  return OK;
}

// what mem and uptime show, must match src/user/lib/moos.h
struct sysinfo
{
  uint32_t heap_used_kb;
  uint32_t heap_total_kb;
  uint32_t heap_used_blocks;
  uint32_t heap_total_blocks;
  uint32_t uptime_ms;
  uint32_t ticks;
  uint32_t tick_hz;
};

static int32_t
sys_sysinfo (struct sysinfo *out)
{
  if (!task_user_range_valid (out, sizeof (*out)))
    {
      return -INVALID_ARG_ERROR;
    }
  struct heap_stats stats;
  kheap_stats (&stats);
  out->heap_used_blocks = stats.used_blocks;
  out->heap_total_blocks = stats.total_blocks;
  out->heap_used_kb = stats.used_blocks * (HEAP_BLOCK_SIZE / 1024);
  out->heap_total_kb = stats.total_blocks * (HEAP_BLOCK_SIZE / 1024);
  out->uptime_ms = pit_uptime_ms ();
  out->ticks = pit_ticks ();
  out->tick_hz = PIT_FREQUENCY_HZ;
  return OK;
}

static int32_t
sys_mutex_open (const char *user_name, uint32_t flags)
{
  char name[MUTEX_NAME_LEN];
  if (sys_copy_string (user_name, name, MUTEX_NAME_LEN - 1) < 0)
    {
      return -INVALID_ARG_ERROR;
    }
  return mutex_open (name, flags);
}

// one program at a time can switch the screen to a graphics mode, the
// text screen comes back when it switches back or exits
static int32_t
sys_gfx_mode (uint32_t mode)
{
  if ((vga_gfx_active () && !task_running->owns_screen)
      || mode > VGA_MODE_MONO)
    {
      return -INVALID_ARG_ERROR;
    }
  if (mode)
    {
      task_running->owns_screen = 1;
      vga_gfx_enter (mode);
    }
  else
    {
      task_running->owns_screen = 0;
      vga_gfx_leave ();
    }
  return OK;
}

static int32_t
sys_gfx_palette (const uint8_t *rgb, uint32_t first, uint32_t count)
{
  if (!task_running->owns_screen || vga_gfx_active () != VGA_MODE_256
      || first > 255 || count > 256 - first
      || !task_user_range_valid (rgb, count * 3))
    {
      return -INVALID_ARG_ERROR;
    }
  vga_gfx_set_palette (rgb, first, count);
  return OK;
}

static int32_t
sys_gfx_blit (const uint8_t *pixels)
{
  if (!task_running->owns_screen
      || !task_user_range_valid (pixels, vga_gfx_frame_size ()))
    {
      return -INVALID_ARG_ERROR;
    }
  vga_gfx_blit (pixels);
  return OK;
}

static int32_t
sys_isatty (int fd)
{
  struct handle *handle = task_fd_get (fd);
  return handle && handle->type == HANDLE_TERMINAL;
}

static int32_t
sys_time (struct rtc_time *out)
{
  if (!task_user_range_valid (out, sizeof (*out)))
    {
      return -INVALID_ARG_ERROR;
    }
  rtc_now (out);
  return OK;
}

void
syscall_handler (struct interrupt_frame *frame)
{
  if (!FRAME_FROM_USER (frame))
    {
      frame->eax = -INVALID_ARG_ERROR;
      return;
    }

  int32_t res = 0;
  switch (frame->eax)
    {
    case SYS_EXIT:
      task_exit ((int32_t)frame->ebx);
      break;
    case SYS_PRINT:
      res = sys_print ((const char *)frame->ebx);
      break;
    case SYS_PUTCHAR:
      {
        char c = frame->ebx;
        struct handle *out = task_fd_get (STDOUT_FD);
        res = out ? handle_write (out, &c, 1) : -INVALID_ARG_ERROR;
      }
      break;
    case SYS_GETCHAR:
      {
        // returns -1 at the end of stdin or if killed while waiting, the
        // task exits on its way back to user mode then
        struct handle *in = task_fd_get (STDIN_FD);
        res = in ? handle_getchar (in) : -1;
      }
      break;
    case SYS_SLEEP:
      task_sleep_ms (frame->ebx);
      break;
    case SYS_SLEEP_UNTIL:
      task_sleep_until_tick (pit_ms_to_ticks (frame->ebx));
      break;
    case SYS_UPTIME:
      res = pit_uptime_ms ();
      break;
    case SYS_YIELD:
      sched_request_resched ();
      break;
    case SYS_GETPID:
      res = task_running->pid;
      break;
    case SYS_SBRK:
      res = task_sbrk ((int32_t)frame->ebx);
      break;
    case SYS_OPEN:
      res = sys_open ((const char *)frame->ebx, frame->ecx);
      break;
    case SYS_READ:
      res = sys_read (frame->ebx, (void *)frame->ecx, frame->edx);
      break;
    case SYS_WRITE:
      res = sys_write (frame->ebx, (const void *)frame->ecx, frame->edx);
      break;
    case SYS_CLOSE:
      res = task_fd_close (frame->ebx);
      break;
    case SYS_FILESIZE:
      res = sys_filesize (frame->ebx);
      break;
    case SYS_LISTDIR:
      res = sys_listdir ((const char *)frame->ebx,
                         (struct file_dirent *)frame->ecx, frame->edx);
      break;
    case SYS_REMOVE:
      res = sys_path_op ((const char *)frame->ebx, remove);
      break;
    case SYS_MKDIR:
      res = sys_path_op ((const char *)frame->ebx, mkdir);
      break;
    case SYS_SEEK:
      res = sys_seek (frame->ebx, frame->ecx, frame->edx);
      break;
    case SYS_PIPE:
      res = sys_pipe ((int *)frame->ebx);
      break;
    case SYS_SPAWN:
      res = sys_spawn ((const char *)frame->ebx, (char **)frame->ecx,
                       (const int *)frame->edx);
      break;
    case SYS_WAIT:
      res = task_wait (frame->ebx);
      break;
    case SYS_ISATTY:
      res = sys_isatty (frame->ebx);
      break;
    case SYS_TIME:
      res = sys_time ((struct rtc_time *)frame->ebx);
      break;
    case SYS_CHDIR:
      res = sys_chdir ((const char *)frame->ebx);
      break;
    case SYS_GETCWD:
      res = sys_getcwd ((char *)frame->ebx, frame->ecx);
      break;
    case SYS_TASKS:
      res = sys_tasks ((struct task_info *)frame->ebx, frame->ecx);
      break;
    case SYS_KILL:
      res = task_kill (frame->ebx);
      break;
    case SYS_SCHED:
      res = sys_sched (frame->ebx);
      break;
    case SYS_SETPRIO:
      res = sys_setprio (frame->ebx, frame->ecx);
      break;
    case SYS_RUN:
      res = sys_run ((const char *)frame->ebx, (char **)frame->ecx,
                     (const struct run_opts *)frame->edx);
      break;
    case SYS_CLEAR:
      terminal_clean ();
      break;
    case SYS_SETCOLOR:
      terminal_setcolour (frame->ebx & 0x0F);
      break;
    case SYS_GETCURSOR:
      res = sys_getcursor ((struct cursor *)frame->ebx);
      break;
    case SYS_SETCURSOR:
      res = sys_setcursor (frame->ebx, frame->ecx);
      break;
    case SYS_SYSINFO:
      res = sys_sysinfo ((struct sysinfo *)frame->ebx);
      break;
    case SYS_MUTEX_OPEN:
      res = sys_mutex_open ((const char *)frame->ebx, frame->ecx);
      break;
    case SYS_MUTEX_LOCK:
      res = mutex_lock (frame->ebx);
      break;
    case SYS_MUTEX_UNLOCK:
      res = mutex_unlock (frame->ebx);
      break;
    case SYS_MUTEX_CLOSE:
      res = mutex_close (frame->ebx);
      break;
    case SYS_PERIOD:
      res = task_set_period (frame->ebx);
      break;
    case SYS_WAIT_PERIOD:
      res = task_wait_period ();
      break;
    case SYS_GFX_MODE:
      res = sys_gfx_mode (frame->ebx);
      break;
    case SYS_GFX_PALETTE:
      res = sys_gfx_palette ((const uint8_t *)frame->ebx, frame->ecx,
                             frame->edx);
      break;
    case SYS_GFX_BLIT:
      res = sys_gfx_blit ((const uint8_t *)frame->ebx);
      break;
    case SYS_KEY_EVENT:
      // only the program in the foreground sees what is typed
      if (frame->ebx)
        {
          keyboard_flush_events ();
        }
      res = task_running->foreground && !frame->ebx ? keyboard_get_event ()
                                                    : -1;
      break;
    case SYS_REBOOT:
      // pulse the cpu reset line through the keyboard controller
      outb (0x64, 0xFE);
      break;
    default:
      res = -INVALID_ARG_ERROR;
      break;
    }
  // popad in isr_common loads this back into eax
  frame->eax = res;
}
