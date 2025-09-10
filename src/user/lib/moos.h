#ifndef MOOS_H
#define MOOS_H

#include <stdint.h>

// system calls
void exit (int32_t code);
int32_t print (const char *str);
void putchar (char c);
// next byte of stdin (a raw key on the terminal), -1 at the end of the input
int getchar (void);
void sleep (uint32_t ms);
uint32_t uptime (void);         // milliseconds since boot
void sleep_until (uint32_t ms); // sleeps until uptime reaches ms
void yield (void);              // gives the cpu to another task
int32_t getpid (void);

void *sbrk (int32_t increment); // grows the heap, returns 0 when out of memory

// files. paths are relative to the working directory the program started in
#define O_READ 0   // read only
#define O_WRITE 1  // create or truncate, then write
#define O_APPEND 2 // create if missing, then write at the end
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
// standard fds, the terminal unless the shell connected them to a file or
// pipe with < > >> or |
#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define NAME_MAX 63

struct dirent
{
  char name[NAME_MAX + 1]; // long file names up to NAME_MAX chars
  uint32_t size;
  uint8_t is_dir;
  uint16_t date; // fat format: bits 15-9 year - 1980, 8-5 month, 4-0 day
  uint16_t time; // bits 15-11 hour, 10-5 minute, 4-0 second / 2
};

struct datetime
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

int open (const char *path, int mode); // returns fd or a negative error
int read (int fd, void *buf, uint32_t len);
int write (int fd, const void *buf, uint32_t len);
int close (int fd);
int filesize (int fd);
int seek (int fd, int32_t offset, int whence);
int listdir (const char *path, struct dirent *out, int max);
int remove (const char *path); // files and empty directories
int mkdir (const char *path);

// processes. spawn starts a program ("sort" or a path) with argv (argv[0] is
// its name, ends with 0) and returns its pid. io[0] and io[1] become its stdin
// and stdout, 0 passes on our own. wait returns the exit code of a child
int pipe (int fds[2]); // fds[0] reads what is written to fds[1]
int spawn (const char *program, char **argv, const int io[2]);
int wait (int pid);
int isatty (int fd); // 1 if fd is the terminal
int time (struct datetime *now);
// working directory, inherited by the programs a task spawns
int chdir (const char *path);
int getcwd (char *buf, uint32_t len); // returns the length of the path

// tasks, see ps in the shell. must match struct task_info in
// src/kernel/task/task.h
struct task_info
{
  int32_t pid;
  char name[16];
  char state[16];
  uint8_t priority;
  uint8_t background;
  uint8_t is_kernel;
  uint8_t reserved;
  uint32_t cpu_ms;
};
int tasks (struct task_info *out, int max); // returns how many
int kill (int pid); // the task exits the next time it runs

// scheduling policies, sched (SCHED_GET) only returns the current one
#define SCHED_GET (-1)
#define SCHED_RR 0 // round robin
#define SCHED_RT 1 // fixed priority preemptive
int sched (int policy);
// real time priority 1-29 of a task, pid 0 is the calling task
#define PRIO_MIN 1
#define PRIO_MAX 29
#define PRIO_DEFAULT 10
int setprio (int pid, int priority);

// spawn with more control: stdin and stdout are fds of the caller (-1 passes
// on our own), background programs dont get keyboard input and nobody waits
// for them, the kernel prints when they are done
struct run_opts
{
  int32_t stdin;
  int32_t stdout;
  int32_t priority;
  int32_t background;
};
int run (const char *program, char **argv, const struct run_opts *opts);

// terminal. text is drawn in one of the 16 vga colours on black
enum colour
{
  BLACK,
  BLUE,
  GREEN,
  CYAN,
  RED,
  MAGENTA,
  BROWN,
  LIGHT_GREY,
  DARK_GREY,
  LIGHT_BLUE,
  LIGHT_GREEN,
  LIGHT_CYAN,
  LIGHT_RED,
  LIGHT_MAGENTA,
  YELLOW,
  WHITE,
};
#define TERM_WIDTH 80
#define TERM_HEIGHT 25
struct cursor
{
  uint32_t row;
  uint32_t column;
  uint32_t scrolls; // times the screen scrolled up since boot
};
void clear (void);
void setcolor (int colour);
int getcursor (struct cursor *out);
int setcursor (int row, int column);

struct sysinfo
{
  uint32_t heap_used_kb; // kernel heap, user pages come from it too
  uint32_t heap_total_kb;
  uint32_t heap_used_blocks;
  uint32_t heap_total_blocks;
  uint32_t uptime_ms;
  uint32_t ticks; // timer interrupts since boot
  uint32_t tick_hz;
};
int sysinfo (struct sysinfo *out);
void reboot (void);

// heap
void *malloc (uint32_t size);
void *calloc (uint32_t count, uint32_t size);
void *realloc (void *ptr, uint32_t size);
void free (void *ptr);

// strings
uint32_t strlen (const char *str);
int strcmp (const char *a, const char *b);
void *memcpy (void *dest, const void *src, uint32_t len);
void *memset (void *dest, int c, uint32_t len);

// helpers
void print_int (int32_t num);
void print_padded (const char *str, uint32_t width);
void eprint (const char *str); // to stderr
// prints "what: reason" to stderr for a negative error from a system call
void print_error (const char *what, int err);

// reads a line with echo and backspace into buf. returns its length
uint32_t readline (char *buf, uint32_t max);

#endif // !MOOS_H
