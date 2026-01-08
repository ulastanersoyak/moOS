#ifndef MOOS_H
#define MOOS_H

#include <stdarg.h>
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
  uint32_t period_ms; // 0 if the task isnt periodic
  uint32_t jobs;
  uint32_t deadline_misses;
};
int tasks (struct task_info *out, int max); // returns how many
int kill (int pid); // the task exits the next time it runs

// scheduling policies, sched (SCHED_GET) only returns the current one
#define SCHED_GET (-1)
#define SCHED_RR 0 // round robin
#define SCHED_RT 1 // fixed priority preemptive
#define SCHED_EDF 2 // earliest deadline first for periodic tasks
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

// mutexes are shared between programs by name (up to 15 chars). the first
// open creates it with flags, MUTEX_INHERIT turns on priority inheritance:
// the owner runs at the priority of its most important waiter, so a medium
// priority task cant keep a high priority one waiting (priority inversion).
// lock returns -1 if the task got killed while waiting
#define MUTEX_INHERIT 0x1
int mutex_open (const char *name, int flags); // returns an id
int mutex_lock (int id);
int mutex_unlock (int id);
int mutex_close (int id);

// periodic tasks: period starts the first job now, each job ends with
// wait_period, which sleeps until the next job is released. a job has to be
// done before the next one is released (its deadline), wait_period returns
// 1 if it wasnt. ps shows the missed deadlines
int period (uint32_t ms); // 0 makes the task ordinary again
int wait_period (void);

// heap
void *malloc (uint32_t size);
void *calloc (uint32_t count, uint32_t size);
void *realloc (void *ptr, uint32_t size);
void free (void *ptr);

// strings
uint32_t strlen (const char *str);
int strcmp (const char *a, const char *b);
int strncmp (const char *a, const char *b, uint32_t n);
int strncasecmp (const char *a, const char *b, uint32_t n);
char *strcpy (char *dest, const char *src);
char *strcat (char *dest, const char *src);
char *strchr (const char *str, int c); // 0 if c isnt in str
void *memcpy (void *dest, const void *src, uint32_t len);
void *memmove (void *dest, const void *src, uint32_t len); // may overlap
void *memset (void *dest, int c, uint32_t len);
int memcmp (const void *a, const void *b, uint32_t len);
int atoi (const char *str);

// chars
int isdigit (int c);
int isalpha (int c);
int isspace (int c);
int isprint (int c);
int tolower (int c);
int toupper (int c);

// formatted output to stdout: %s %d %u %x %c and %%, with a width like %5d
// or %-12s (left aligned). snprintf always terminates the string and returns
// the length it would have had
int printf (const char *fmt, ...);
int vprintf (const char *fmt, va_list args);
int snprintf (char *buf, uint32_t size, const char *fmt, ...);
int vsnprintf (char *buf, uint32_t size, const char *fmt, va_list args);

// helpers
void print_int (int32_t num);
void print_padded (const char *str, uint32_t width);
void eprint (const char *str); // to stderr
// prints "what: reason" to stderr for a negative error from a system call
void print_error (const char *what, int err);

// busy work for the real time demos: busy_ms takes ms of cpu time however
// often the task gets preempted in between. busy_calibrate measures how many
// loops make a millisecond, run it once at a high priority first
uint32_t busy_calibrate (void);
void busy_ms (uint32_t ms, uint32_t loops_per_ms);

// reads a line with echo and backspace into buf. returns its length
uint32_t readline (char *buf, uint32_t max);

#endif // !MOOS_H
