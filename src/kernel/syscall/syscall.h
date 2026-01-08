#ifndef SYSCALL_H
#define SYSCALL_H

#include "../idt/idt.h"

// system call numbers, passed in eax. arguments are passed in ebx, ecx, edx
// and the result is returned in eax. must match src/user/lib/syscall.asm
enum
{
  SYS_EXIT = 0,        // exit (int32_t code)
  SYS_PRINT = 1,       // print (const char *str)
  SYS_PUTCHAR = 2,     // putchar (char c)
  SYS_GETCHAR = 3,     // int getchar (void), next byte of stdin or -1
  SYS_SLEEP = 4,       // sleep (uint32_t ms)
  SYS_UPTIME = 5,      // uint32_t uptime (void), milliseconds since boot
  SYS_SLEEP_UNTIL = 6, // sleep_until (uint32_t ms), absolute uptime in ms
  SYS_YIELD = 7,       // yield (void), gives up the rest of the time slice
  SYS_GETPID = 8,      // int32_t getpid (void)
  SYS_OPEN = 9,        // int open (const char *path, int mode), see moos.h
  SYS_READ = 10,       // int read (int fd, void *buf, uint32_t len)
  SYS_WRITE = 11,      // int write (int fd, const void *buf, uint32_t len)
  SYS_CLOSE = 12,      // int close (int fd)
  SYS_FILESIZE = 13,   // int filesize (int fd)
  SYS_LISTDIR = 14,    // int listdir (path, struct dirent *out, int max)
  SYS_SBRK = 15,       // void *sbrk (int32_t increment)
  SYS_REMOVE = 16,     // int remove (const char *path)
  SYS_MKDIR = 17,      // int mkdir (const char *path)
  SYS_SEEK = 18,       // int seek (int fd, int32_t offset, int whence)
  SYS_PIPE = 19,       // int pipe (int fds[2]), read end in 0, write in 1
  SYS_SPAWN = 20,      // int spawn (path, char **argv, const int io[2])
  SYS_WAIT = 21,       // int wait (int pid), returns the exit code
  SYS_ISATTY = 22,     // int isatty (int fd)
  SYS_TIME = 23,       // int time (struct datetime *out)
  SYS_CHDIR = 24,      // int chdir (const char *path)
  SYS_GETCWD = 25,     // int getcwd (char *buf, uint32_t len)
  SYS_TASKS = 26,      // int tasks (struct task_info *out, int max)
  SYS_KILL = 27,       // int kill (int pid)
  SYS_SCHED = 28,      // int sched (int policy), -1 only returns it
  SYS_SETPRIO = 29,    // int setprio (int pid, int priority), 0 is self
  SYS_RUN = 30,        // int run (path, char **argv, struct run_opts *opts)
  SYS_CLEAR = 31,      // clear (void), clears the screen
  SYS_SETCOLOR = 32,   // setcolor (int colour), vga colour of new text
  SYS_GETCURSOR = 33,  // int getcursor (struct cursor *out)
  SYS_SETCURSOR = 34,  // setcursor (int row, int column)
  SYS_SYSINFO = 35,    // int sysinfo (struct sysinfo *out)
  SYS_REBOOT = 36,     // reboot (void)
  SYS_MUTEX_OPEN = 37,   // int mutex_open (const char *name, int flags)
  SYS_MUTEX_LOCK = 38,   // int mutex_lock (int id)
  SYS_MUTEX_UNLOCK = 39, // int mutex_unlock (int id)
  SYS_MUTEX_CLOSE = 40,  // int mutex_close (int id)
  SYS_PERIOD = 41,       // int period (uint32_t ms), 0 stops
  SYS_WAIT_PERIOD = 42,  // int wait_period (void), 1 if the job was late
};

// handles int 0x80 from user tasks
void syscall_handler (struct interrupt_frame *frame);

#endif // !SYSCALL_H
