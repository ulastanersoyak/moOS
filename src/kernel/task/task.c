#include "task.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga_gfx.h"
#include "../../drivers/timer/pit.h"
#include "../../file_system/path_parser.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/stdlib/stdlib.h"
#include "../../libc/string/string.h"
#include "../io/io.h"
#include "../kpanic.h"
#include "elf.h"
#include "mutex.h"
#include "sched.h"

// asm functions
void task_user_start (void);
void task_kernel_start (void);
uint32_t irq_save (void);
void irq_restore (uint32_t eflags);

struct task task_table[MAX_TASKS];
struct task *task_running = 0;
struct task *task_idle = 0;

// the boot context, adopts the children of tasks that exit
static struct task *task_init_thread = 0;
static struct page_dir *kernel_page_dir = 0;
static int32_t next_pid = 0;

static const char *state_names[] = {
  "unused", "loading", "ready",   "running", "sleeping",
  "key",    "wait",    "blocked", "zombie",
};

const char *
task_state_name (enum task_state state)
{
  return state_names[state];
}

struct page_dir *
task_kernel_dir (void)
{
  return kernel_page_dir;
}

// takes a free slot. the caller sets the final state once the task is set up
static struct task *
task_alloc (const char *name, uint8_t priority, uint8_t is_kernel)
{
  uint32_t flags = irq_save ();
  struct task *task = 0;
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      if (task_table[i].state == TASK_UNUSED)
        {
          task = &task_table[i];
          memset (task, 0, sizeof (struct task));
          task->state = TASK_LOADING;
          task->pid = next_pid++;
          break;
        }
    }
  irq_restore (flags);
  if (!task)
    {
      return 0;
    }
  uint32_t len = strlen (name);
  if (len >= TASK_NAME_LEN)
    {
      len = TASK_NAME_LEN - 1;
    }
  memcpy (task->name, name, len);
  task->priority = priority;
  task->base_priority = priority;
  task->is_kernel = is_kernel;
  fpu_init_state (&task->fpu);
  task->wait_pid = -1;
  return task;
}

// frees every page mapped in the user page tables: code, data, heap, stack
static void
task_free_user_pages (struct task *task)
{
  for (uint32_t t = 0; t < USER_SPACE_TABLES; t++)
    {
      uint32_t *table = task->dir->user_tables[t];
      for (uint32_t i = 0; table && i < TOTAL_ENTRIES_PER_TABLE; i++)
        {
          if (table[i] & IS_PRESENT)
            {
              free ((void *)(table[i] & 0xfffff000));
            }
        }
    }
}

static void
task_free (struct task *task)
{
  if (task->dir)
    {
      task_free_user_pages (task);
      page_dir_free_user (task->dir);
    }
  free (task->kernel_stack);
  task->dir = 0;
  task->kernel_stack = 0;
  task->state = TASK_UNUSED;
}

void
task_reap_zombies (void)
{
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      // zombies with a parent stay until task_wait collects the exit code
      if (task->state == TASK_ZOMBIE && task->detached && task != task_running)
        {
          task_free (task);
        }
    }
}

// builds the stack that context_switch pops on the first switch to the task:
// edi, esi, ebx, ebp and the return address
static uint32_t *
task_push_context (uint32_t *sp, uint32_t entry, uint32_t ebx)
{
  *--sp = entry; // return address of context_switch
  *--sp = 0;     // ebp
  *--sp = ebx;   // ebx
  *--sp = 0;     // esi
  *--sp = 0;     // edi
  return sp;
}

static int32_t
task_alloc_kernel_stack (struct task *task)
{
  task->kernel_stack = calloc (TASK_KERNEL_STACK_SIZE);
  if (!task->kernel_stack)
    {
      return -NO_MEMORY_ERROR;
    }
  task->kernel_stack_top
      = (uint32_t)task->kernel_stack + TASK_KERNEL_STACK_SIZE;
  return OK;
}

static void
idle_main (void)
{
  for (;;)
    {
      // halts until an interrupt, then runs whatever became ready
      cpu_wait ();
      uint32_t flags = irq_save ();
      schedule ();
      irq_restore (flags);
    }
}

void
task_init (struct page_dir *kernel_dir, uint8_t verbose)
{
  kernel_page_dir = kernel_dir;
  memset (task_table, 0, sizeof (task_table));

  task_idle = task_alloc ("idle", 0, 1);
  task_alloc_kernel_stack (task_idle);
  uint32_t *sp = (uint32_t *)task_idle->kernel_stack_top;
  sp = task_push_context (sp, (uint32_t)task_kernel_start,
                          (uint32_t)idle_main);
  task_idle->context_esp = (uint32_t)sp;
  task_idle->state = TASK_READY;

  // whatever runs kernel_main from now on is the init thread
  task_init_thread = task_alloc ("init", TASK_PRIORITY_SHELL, 1);
  task_init_thread->state = TASK_RUNNING;
  strcpy (task_init_thread->cwd, "0:/");
  task_running = task_init_thread;
  task_init_thread->foreground = 1;

  if (verbose)
    {
      printf ("multitasking init <%s>",
              sched_policy_name (sched_get_policy ()));
      init_OK ();
    }
}

// returns a kernel pointer to the user page that contains vaddr, mapping a new
// zeroed page first if needed. pages only become read only if every part
// loaded into them is read only
static uint8_t *
task_user_page (struct task *task, uint32_t vaddr, uint8_t writable)
{
  uint32_t *entry = page_dir_user_entry (task->dir, vaddr, 1);
  if (!entry)
    {
      return 0;
    }
  if (!(*entry & IS_PRESENT))
    {
      void *page = calloc (PAGE_SIZE);
      if (!page)
        {
          return 0;
        }
      *entry = (uint32_t)page | IS_PRESENT | ACCESS_ALL;
    }
  if (writable)
    {
      *entry |= IS_WRITABLE;
    }
  return (uint8_t *)(*entry & 0xfffff000);
}

// copies src (or nothing if src is 0) to the user address space of task.
// works on any task since it goes through the page table, not cr3
static int32_t
task_copy_to_user (void *ctx, uint32_t vaddr, const void *src, uint32_t len,
                   uint8_t writable)
{
  struct task *task = ctx;
  const uint8_t *in = src;
  while (len > 0)
    {
      uint8_t *page = task_user_page (task, vaddr, writable);
      if (!page)
        {
          return -NO_MEMORY_ERROR;
        }
      uint32_t offset = vaddr % PAGE_SIZE;
      uint32_t chunk = PAGE_SIZE - offset;
      if (chunk > len)
        {
          chunk = len;
        }
      if (in)
        {
          memcpy (page + offset, in, chunk);
          in += chunk;
        }
      vaddr += chunk;
      len -= chunk;
    }
  return OK;
}

static int32_t
task_load (struct task *task, const char *path)
{
  int fd = fopen (path, "r");
  if (!fd)
    {
      return -NOT_FOUND_ERROR;
    }
  int32_t res = OK;
  uint8_t *buf = 0;
  struct file_stat stat;
  if (fstat (fd, &stat) != OK || stat.filesize == 0)
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }
  task->dir = page_dir_new_user (kernel_page_dir, (void *)USER_CODE_ADDR);
  buf = malloc (stat.filesize);
  if (!task->dir || !buf)
    {
      res = -NO_MEMORY_ERROR;
      goto out;
    }
  if (fread (buf, stat.filesize, 1, fd) != 1)
    {
      res = -IO_ERROR;
      goto out;
    }
  uint32_t image_end = 0;
  res = elf_load (buf, stat.filesize, USER_CODE_ADDR, USER_HEAP_LIMIT,
                  task_copy_to_user, task, &task->entry, &image_end);
  if (res < 0)
    {
      goto out;
    }
  task->brk_start = (image_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  task->brk = task->brk_start;
  // stack pages, the page below them stays unmapped to catch overflows
  res = task_copy_to_user (task, USER_STACK_TOP - USER_STACK_SIZE, 0,
                           USER_STACK_SIZE, 1);
out:
  free (buf);
  fclose (fd);
  return res;
}

// copies argv to the top of the user stack and returns the initial stack
// pointer, which points at argc followed by argv like a cdecl call to main
static int32_t
task_push_args (struct task *task, int argc, char **argv, uint32_t *esp)
{
  if (argc > TASK_MAX_ARGS)
    {
      return -INVALID_ARG_ERROR;
    }
  uint32_t sp = USER_STACK_TOP;
  uint32_t pointers[TASK_MAX_ARGS + 1];
  for (int i = argc - 1; i >= 0; i--)
    {
      uint32_t len = strlen (argv[i]) + 1;
      sp -= len;
      if (USER_STACK_TOP - sp > TASK_MAX_ARGS_BYTES)
        {
          return -INVALID_ARG_ERROR;
        }
      task_copy_to_user (task, sp, argv[i], len, 1);
      pointers[i] = sp;
    }
  pointers[argc] = 0; // argv ends with a null pointer
  sp &= ~3;
  sp -= (argc + 1) * sizeof (uint32_t);
  task_copy_to_user (task, sp, pointers, (argc + 1) * sizeof (uint32_t), 1);
  uint32_t frame[2] = { argc, sp };
  sp -= sizeof (frame);
  task_copy_to_user (task, sp, frame, sizeof (frame), 1);
  *esp = sp;
  return OK;
}

static struct handle *task_inherit_fd (int fd);

// "0:/bin/hello.bin" -> "hello"
static void
task_name_from_path (const char *path, char *out)
{
  const char *start = path;
  for (const char *c = path; *c; c++)
    {
      if (*c == '/')
        {
          start = c + 1;
        }
    }
  uint32_t len = 0;
  while (start[len] && start[len] != '.' && len < TASK_NAME_LEN - 1)
    {
      out[len] = start[len];
      len++;
    }
  out[len] = '\0';
}

int32_t
task_spawn (const char *path, int argc, char **argv, uint8_t priority,
            uint32_t flags, struct handle *stdin, struct handle *stdout)
{
  uint8_t background = (flags & TASK_SPAWN_BACKGROUND) != 0;
  char name[TASK_NAME_LEN];
  task_name_from_path (path, name);
  struct task *task = task_alloc (name, priority, 0);
  if (!task)
    {
      return -NO_MEMORY_ERROR;
    }
  uint32_t user_esp = 0;
  int32_t res = task_load (task, path);
  if (res == OK)
    {
      res = task_push_args (task, argc, argv, &user_esp);
    }
  if (res == OK)
    {
      res = task_alloc_kernel_stack (task);
    }
  if (res < 0)
    {
      task_free (task);
      return res;
    }

  // first switch to the task "returns" into task_user_start, which irets
  // to the program entry point in ring 3
  uint32_t *sp = (uint32_t *)task->kernel_stack_top;
  *--sp = USER_DATA_SEG; // ss
  *--sp = user_esp;      // esp
  *--sp = 0x202;         // eflags, interrupts enabled
  *--sp = USER_CODE_SEG; // cs
  *--sp = task->entry;   // eip
  sp = task_push_context (sp, (uint32_t)task_user_start, 0);
  task->context_esp = (uint32_t)sp;
  task->parent = task_running;
  task->background = background;
  task->detached = background;
  task->is_shell = (flags & TASK_SPAWN_SHELL) != 0;
  task->foreground = !background && task_running->foreground;
  strcpy (task->cwd, task_running->cwd);
  task->fds[0] = handle_ref (stdin ? stdin : task_inherit_fd (0));
  task->fds[1] = handle_ref (stdout ? stdout : task_inherit_fd (1));
  task->fds[2] = handle_ref (task_inherit_fd (2));

  uint32_t eflags = irq_save ();
  if (task->foreground)
    {
      // drop keys typed before the program started
      keyboard_flush ();
    }
  sched_wake (task);
  irq_restore (eflags);
  return task->pid;
}

uint32_t
task_sbrk (int32_t increment)
{
  struct task *task = task_running;
  uint32_t old_brk = task->brk;
  uint32_t new_brk = old_brk + increment;
  if ((increment > 0 && new_brk < old_brk) || new_brk < task->brk_start
      || new_brk > USER_HEAP_LIMIT)
    {
      return 0;
    }
  // map the pages between the old and new end, shrinking keeps them mapped
  if (new_brk > old_brk
      && task_copy_to_user (task, old_brk, 0, new_brk - old_brk, 1) < 0)
    {
      return 0;
    }
  task->brk = new_brk;
  return old_brk;
}

const char *
task_cwd (void)
{
  return task_running->cwd;
}

void
task_set_cwd (const char *path)
{
  strcpy (task_running->cwd, path);
}

int
task_resolve_path (const char *path, char *out)
{
  return pathparser_resolve (task_running->cwd, path, out, MAX_PATH_LEN + 1);
}

// fd of the running task for a new child, the terminal for kernel threads
static struct handle *
task_inherit_fd (int fd)
{
  struct handle *handle = task_fd_get (fd);
  return handle ? handle : handle_terminal ();
}

int
task_fd_install (struct handle *handle)
{
  for (int i = 0; i < TASK_MAX_FDS; i++)
    {
      if (!task_running->fds[i])
        {
          task_running->fds[i] = handle;
          return i;
        }
    }
  return -FILE_HANDLER_OVERFLOW;
}

struct handle *
task_fd_get (int fd)
{
  if (task_running->is_kernel)
    {
      return fd >= 0 && fd <= 2 ? handle_terminal () : 0;
    }
  if (fd < 0 || fd >= TASK_MAX_FDS)
    {
      return 0;
    }
  return task_running->fds[fd];
}

int
task_fd_close (int fd)
{
  struct handle *handle = task_fd_get (fd);
  if (!handle || task_running->is_kernel)
    {
      return -INVALID_ARG_ERROR;
    }
  task_running->fds[fd] = 0;
  handle_close (handle);
  return OK;
}

int
task_find_program (const char *name, char *path)
{
  if (strchr (name, '/') || strchr (name, ':'))
    {
      return task_resolve_path (name, path);
    }
  if (strlen (TASK_BIN_DIR) + strlen (name) + strlen (TASK_BIN_EXT)
      > MAX_PATH_LEN)
    {
      return 0;
    }
  strcpy (path, TASK_BIN_DIR);
  strcat (path, name);
  strcat (path, TASK_BIN_EXT);
  int fd = fopen (path, "r");
  if (!fd)
    {
      return 0;
    }
  fclose (fd);
  return 1;
}

static struct task *
task_find (int32_t pid, uint8_t zombies)
{
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->pid == pid && task->state != TASK_UNUSED
          && task->state != TASK_LOADING
          && (zombies || task->state != TASK_ZOMBIE))
        {
          return task;
        }
    }
  return 0;
}

int32_t
task_wait (int32_t pid)
{
  uint32_t flags = irq_save ();
  struct task *child = task_find (pid, 1);
  if (!child || child->parent != task_running || child->detached)
    {
      irq_restore (flags);
      return -INVALID_ARG_ERROR;
    }
  // the child wakes its parent when it exits
  while (child->state != TASK_ZOMBIE)
    {
      task_running->wait_pid = pid;
      task_running->state = TASK_WAITING_CHILD;
      schedule ();
    }
  task_running->wait_pid = -1;
  int32_t exit_code = child->exit_code;
  task_free (child);
  irq_restore (flags);
  return exit_code;
}

void
task_exit (int32_t exit_code)
{
  struct task *task = task_running;
  if (task->is_kernel)
    {
      panic ("task_exit: kernel threads cant exit\n");
    }
  // tasks waiting for its mutexes get them now
  mutex_release_all (task);
  // a program that dies in a graphics mode gives the text screen back
  if (task->owns_screen)
    {
      task->owns_screen = 0;
      vga_gfx_leave ();
    }
  // closing files flushes writes, closing pipe ends wakes up the other side
  for (int i = 0; i < TASK_MAX_FDS; i++)
    {
      if (task->fds[i])
        {
          handle_close (task->fds[i]);
          task->fds[i] = 0;
        }
    }
  // never restored, the task doesnt run again
  irq_save ();
  task->state = TASK_ZOMBIE;
  task->exit_code = exit_code;

  // children outlive their parent, nobody will wait for them anymore
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      if (task_table[i].state != TASK_UNUSED && task_table[i].parent == task)
        {
          task_table[i].parent = task_init_thread;
          task_table[i].detached = 1;
        }
    }

  struct task *parent = task->parent;
  if (parent && parent->state == TASK_WAITING_CHILD
      && parent->wait_pid == task->pid)
    {
      sched_wake (parent);
    }
  if (task->background)
    {
      printf ("[%d] %s %s\n", task->pid, task->name,
              exit_code == TASK_KILLED ? "killed" : "done");
    }
  if (task->foreground)
    {
      // drop input meant for the program (e.g. the ctrl+c that killed it)
      keyboard_flush ();
    }
  schedule ();
  panic ("task_exit: zombie task was scheduled\n");
}

int32_t
task_kill (int32_t pid)
{
  uint32_t flags = irq_save ();
  struct task *task = task_find (pid, 0);
  if (!task || task->is_kernel)
    {
      irq_restore (flags);
      return -INVALID_ARG_ERROR;
    }
  task->killed = 1;
  // a killed task has to run once to exit, make sure it gets the cpu even
  // if higher priority tasks keep the real time scheduler busy
  task->priority = TASK_PRIORITY_MAX;
  if (task->state == TASK_SLEEPING || task->state == TASK_WAITING_KEY
      || task->state == TASK_BLOCKED)
    {
      sched_wake (task);
    }
  irq_restore (flags);
  return OK;
}

void
task_kill_foreground (void)
{
  uint8_t killed = 0;
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->foreground && !task->is_kernel && !task->is_shell
          && task->state != TASK_UNUSED
          && task->state != TASK_ZOMBIE && task_kill (task->pid) == OK)
        {
          killed = 1;
        }
    }
  if (killed)
    {
      printf ("^C\n");
      keyboard_flush ();
    }
}

void
task_block_on (void *channel)
{
  task_running->wait_channel = channel;
  task_running->state = TASK_BLOCKED;
  schedule ();
  task_running->wait_channel = 0;
}

void
task_wake_channel (void *channel)
{
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->state == TASK_BLOCKED && task->wait_channel == channel)
        {
          sched_wake (task);
        }
    }
}

void
task_sleep_until_tick (uint32_t tick)
{
  uint32_t flags = irq_save ();
  while ((int32_t)(tick - pit_ticks ()) > 0 && !task_running->killed)
    {
      task_running->wake_tick = tick;
      task_running->state = TASK_SLEEPING;
      schedule ();
    }
  irq_restore (flags);
}

void
task_sleep_ms (uint32_t ms)
{
  task_sleep_until_tick (pit_ticks () + pit_ms_to_ticks (ms));
}

int32_t
task_wait_key (void)
{
  uint32_t flags = irq_save ();
  int32_t c = -1;
  while (!task_running->killed)
    {
      if (task_running->foreground && (c = keyboard_getchar ()) >= 0)
        {
          break;
        }
      task_running->state = TASK_WAITING_KEY;
      schedule ();
    }
  irq_restore (flags);
  return task_running->killed ? -1 : c;
}

void
task_on_keyboard_input (void)
{
  for (uint32_t i = 0; i < MAX_TASKS; i++)
    {
      struct task *task = &task_table[i];
      if (task->foreground && task->state == TASK_WAITING_KEY)
        {
          sched_wake (task);
        }
    }
}

void
task_return_to_user (void)
{
  if (task_running->killed)
    {
      task_exit (TASK_KILLED);
    }
  if (sched_need_resched ())
    {
      schedule ();
    }
  // may have been killed while other tasks ran
  if (task_running->killed)
    {
      task_exit (TASK_KILLED);
    }
}

int
task_list (struct task_info *out, int max)
{
  int count = 0;
  uint32_t flags = irq_save ();
  for (uint32_t i = 0; i < MAX_TASKS && count < max; i++)
    {
      struct task *task = &task_table[i];
      if (task->state == TASK_UNUSED)
        {
          continue;
        }
      struct task_info *info = &out[count++];
      memset (info, 0, sizeof (*info));
      info->pid = task->pid;
      strcpy (info->name, task->name);
      strcpy (info->state, task_state_name (task->state));
      info->priority = task->priority;
      info->background = task->background;
      info->is_kernel = task->is_kernel;
      info->cpu_ms = task->cpu_ticks * (1000 / PIT_FREQUENCY_HZ);
      info->period_ms = task->period_ticks * (1000 / PIT_FREQUENCY_HZ);
      info->jobs = task->jobs;
      info->deadline_misses = task->deadline_misses;
    }
  irq_restore (flags);
  return count;
}

int32_t
task_set_period (uint32_t ms)
{
  struct task *task = task_running;
  if (task->is_kernel)
    {
      return -INVALID_ARG_ERROR;
    }
  uint32_t flags = irq_save ();
  task->period_ticks = pit_ms_to_ticks (ms);
  task->deadline = pit_ticks () + task->period_ticks;
  task->jobs = 0;
  task->deadline_misses = 0;
  // the earliest deadline first policy orders tasks by their deadline
  sched_request_resched ();
  irq_restore (flags);
  return OK;
}

int32_t
task_wait_period (void)
{
  struct task *task = task_running;
  if (!task->period_ticks)
    {
      return -INVALID_ARG_ERROR;
    }
  uint32_t flags = irq_save ();
  // a job that ends in the tick of its deadline already ended too late
  int32_t missed = (int32_t)(pit_ticks () - task->deadline) >= 0;
  task->jobs++;
  task->deadline_misses += missed;
  uint32_t release = task->deadline;
  task->deadline += task->period_ticks;
  irq_restore (flags);
  task_sleep_until_tick (release);
  return missed;
}

int32_t
task_set_priority (int32_t pid, uint8_t priority)
{
  uint32_t flags = irq_save ();
  struct task *task = pid ? task_find (pid, 0) : task_running;
  if (!task || task->is_kernel || task->killed)
    {
      irq_restore (flags);
      return -INVALID_ARG_ERROR;
    }
  task->base_priority = priority;
  mutex_update_priority (task);
  irq_restore (flags);
  return OK;
}

uint8_t
task_user_range_valid (const void *ptr, uint32_t len)
{
  uint32_t start = (uint32_t)ptr;
  uint32_t end = start + len;
  return !task_running->is_kernel && start >= USER_CODE_ADDR && end >= start
         && end <= USER_STACK_TOP;
}
