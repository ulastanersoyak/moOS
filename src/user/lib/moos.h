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
