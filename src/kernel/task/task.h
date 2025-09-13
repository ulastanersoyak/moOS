#ifndef TASK_H
#define TASK_H

#include "../config.h"
#include "../memory/paging/page.h"
#include "handle.h"
#include <stdint.h>

// exit code of a task killed by an exception, ctrl+c or kill
#define TASK_KILLED (-1)

// task_spawn flags
#define TASK_SPAWN_BACKGROUND 0x1 // started with &, nobody waits for it
#define TASK_SPAWN_SHELL 0x2      // ctrl+c doesnt kill it

enum task_state
{
  TASK_UNUSED,
  TASK_LOADING, // slot taken, program still being loaded
  TASK_READY,
  TASK_RUNNING,
  TASK_SLEEPING,
  TASK_WAITING_KEY,
  TASK_WAITING_CHILD,
  TASK_BLOCKED, // waits on a channel, e.g. a pipe, see task_block_on
  TASK_ZOMBIE,  // exited, kept until the parent collects the exit code
};

struct task
{
  int32_t pid;
  char name[TASK_NAME_LEN];
  enum task_state state;
  uint8_t priority;   // only used by the real time policy
  uint8_t is_kernel;  // kernel threads have no user address space
  uint8_t background; // started with &, nobody waits for it
  uint8_t foreground; // receives keyboard input and ctrl+c
  uint8_t detached;   // nobody will wait for it, freed as soon as it exits
  uint8_t is_shell;   // in the foreground but ctrl+c doesnt kill it
  volatile uint8_t killed;

  // saved kernel stack pointer while the task is not running
  uint32_t context_esp;
  void *kernel_stack; // 0 for the shell, which runs on the boot stack
  uint32_t kernel_stack_top;

  // user address space. every user page is allocated separately and found
  // through the user page table when the task is freed
  struct page_dir *dir;
  uint32_t entry;     // program entry point from the elf header
  uint32_t brk_start; // user heap, grows with sbrk
  uint32_t brk;

  // file descriptors, 0 stdin, 1 stdout, 2 stderr. 0 is a closed fd
  struct handle *fds[TASK_MAX_FDS];
  char cwd[MAX_PATH_LEN + 1];

  uint32_t wake_tick; // when TASK_SLEEPING
  int32_t slice;      // ticks left before round robin switches task
  uint32_t cpu_ticks; // timer ticks spent running

  struct task *parent;
  int32_t wait_pid;   // child the task waits for in TASK_WAITING_CHILD
  void *wait_channel; // what the task waits for in TASK_BLOCKED
  int32_t exit_code;  // set when it becomes a zombie
};

// shared with the scheduler
extern struct task task_table[MAX_TASKS];
extern struct task *task_running;
extern struct task *task_idle;

// turns the boot context into the init thread and starts the idle thread
void task_init (struct page_dir *kernel_dir, uint8_t verbose);

// page directory used by kernel threads
struct page_dir *task_kernel_dir (void);

// loads the elf executable at path and makes it ready to run in ring 3 with
// argc/argv as arguments of main. stdin and stdout become fd 0 and 1 of the
// task (0 inherits the fd of the running task), the task takes its own
// reference. flags are TASK_SPAWN_*. returns its pid or a negative error.
// tasks that dont run in the background receive keyboard input if their
// parent does
int32_t task_spawn (const char *path, int argc, char **argv, uint8_t priority,
                    uint32_t flags, struct handle *stdin,
                    struct handle *stdout);

// finds the program for a command: "hello" becomes 0:/bin/hello.elf, names
// with a / or : are paths. returns 0 if there is no such file
int task_find_program (const char *name, char *path);

// grows (or shrinks) the user heap of the running task by increment bytes.
// returns the previous end of the heap or 0 if there is no memory left
uint32_t task_sbrk (int32_t increment);

// working directory of the running task, relative paths start there
const char *task_cwd (void);
void task_set_cwd (const char *path);

// resolves path against the working directory of the running task into a
// normalized absolute path. returns 0 if the result is too long
int task_resolve_path (const char *path, char *out);

// fd table of the running task. install takes over the reference and
// returns the lowest free fd, get returns 0 for closed fds. kernel threads
// have no table, their fds 0-2 are the terminal
int task_fd_install (struct handle *handle);
struct handle *task_fd_get (int fd);
int task_fd_close (int fd);

// waits until the child with pid exits and returns its exit code. negative
// errors other than TASK_KILLED mean pid is not a child of the running task
int32_t task_wait (int32_t pid);

// ends the running user task. never returns
void task_exit (int32_t exit_code);

// marks a user task to exit with TASK_KILLED the next time it runs
int32_t task_kill (int32_t pid);

// ctrl+c, kills every foreground user task (a whole pipeline) except the
// shell
void task_kill_foreground (void);

// waits until task_wake_channel is called with the same channel, or the task
// gets killed. interrupts must be disabled, callers check their condition
// again after waking up
void task_block_on (void *channel);
void task_wake_channel (void *channel);

// blocking waits, they let other tasks run in the meantime
void task_sleep_ms (uint32_t ms);
void task_sleep_until_tick (uint32_t tick);
// returns the next key typed while the task is in the foreground, or -1 if
// the task got killed while waiting
int32_t task_wait_key (void);

// called from the keyboard irq after a char was buffered
void task_on_keyboard_input (void);

// called before returning to ring 3. exits killed tasks and switches task if
// the scheduler asked for it
void task_return_to_user (void);

// frees resources of exited tasks, called by the scheduler
void task_reap_zombies (void);

// true if [ptr, ptr + len) lies inside the running user task's address space
uint8_t task_user_range_valid (const void *ptr, uint32_t len);

const char *task_state_name (enum task_state state);

// what ps shows about a task, filled by task_list
struct task_info
{
  int32_t pid;
  char name[TASK_NAME_LEN];
  char state[TASK_NAME_LEN];
  uint8_t priority;
  uint8_t background;
  uint8_t is_kernel;
  uint8_t reserved;
  uint32_t cpu_ms;
};

// fills out with up to max tasks, returns how many
int task_list (struct task_info *out, int max);

// sets the real time priority of a user task, pid 0 is the running task
int32_t task_set_priority (int32_t pid, uint8_t priority);

#endif // !TASK_H
