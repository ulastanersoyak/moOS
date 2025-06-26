#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_SIZE 4096

// one way byte stream between tasks. readers block while it is empty,
// writers while it is full
struct pipe
{
  char buffer[PIPE_SIZE];
  uint32_t head;  // next byte to read
  uint32_t count; // bytes buffered
  uint32_t readers;
  uint32_t writers;
};

// returns a pipe with one reader and one writer, or 0 when out of memory
struct pipe *pipe_new (void);

// reads up to len bytes, waiting for data. returns 0 at the end of the stream
// (empty and no writers left) and -1 if the task got killed while waiting
int32_t pipe_read (struct pipe *pipe, void *buf, uint32_t len);

// writes len bytes, waiting for space. returns the bytes written or a
// negative error if no reader is left
int32_t pipe_write (struct pipe *pipe, const void *buf, uint32_t len);

// drops one end, the pipe is freed when both ends are closed
void pipe_close_reader (struct pipe *pipe);
void pipe_close_writer (struct pipe *pipe);

#endif // !PIPE_H
