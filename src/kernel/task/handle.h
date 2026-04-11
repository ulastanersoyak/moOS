#ifndef HANDLE_H
#define HANDLE_H

#include <stdint.h>

// what a file descriptor of a task refers to
enum handle_type
{
  HANDLE_TERMINAL,   // keyboard in, screen out
  HANDLE_FILE,       // a file opened with fopen
  HANDLE_PIPE_READ,  // read end of a pipe
  HANDLE_PIPE_WRITE, // write end of a pipe
};

struct pipe;

// handles are reference counted so tasks can share them, e.g. the write end
// of a pipe inherited by a child. the last close releases the file or pipe end
struct handle
{
  enum handle_type type;
  uint32_t refs;
  int file_fd; // kernel fd for HANDLE_FILE
  struct pipe *pipe;
};

// the terminal handle, shared by every task and never freed
struct handle *handle_terminal (void);

// wraps a kernel fd from fopen, the handle closes it. 0 when out of memory
struct handle *handle_new_file (int file_fd);

// creates a pipe and returns 0 with a handle for each end
int32_t handle_new_pipe (struct handle **read_end, struct handle **write_end);

// adds a reference and returns the handle
struct handle *handle_ref (struct handle *handle);

// drops a reference
void handle_close (struct handle *handle);

// reads up to len bytes, returns 0 at the end of the file or stream. the
// terminal reads a whole line with echo and backspace, ctrl+d ends the input
int32_t handle_read (struct handle *handle, void *buf, uint32_t len);

int32_t handle_write (struct handle *handle, const void *buf, uint32_t len);

// bytes a read takes without waiting: what a pipe holds, -1 once it is
// empty and nobody can write to it anymore. only for the read end of pipes
int32_t handle_pending (struct handle *handle);

// one byte without line editing: the raw key for the terminal. returns -1 at
// the end of the stream
int32_t handle_getchar (struct handle *handle);

#endif // !HANDLE_H
