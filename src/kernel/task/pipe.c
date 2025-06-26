#include "pipe.h"
#include "../../libc/stdlib/stdlib.h"
#include "../config.h"
#include "task.h"

// asm functions
uint32_t irq_save (void);
void irq_restore (uint32_t eflags);

struct pipe *
pipe_new (void)
{
  struct pipe *pipe = calloc (sizeof (struct pipe));
  if (pipe)
    {
      pipe->readers = 1;
      pipe->writers = 1;
    }
  return pipe;
}

int32_t
pipe_read (struct pipe *pipe, void *buf, uint32_t len)
{
  uint32_t flags = irq_save ();
  while (pipe->count == 0 && pipe->writers > 0 && !task_running->killed)
    {
      task_block_on (pipe);
    }
  if (task_running->killed)
    {
      irq_restore (flags);
      return -1;
    }
  uint32_t total = len < pipe->count ? len : pipe->count;
  char *out = buf;
  for (uint32_t i = 0; i < total; i++)
    {
      out[i] = pipe->buffer[pipe->head];
      pipe->head = (pipe->head + 1) % PIPE_SIZE;
    }
  pipe->count -= total;
  // writers may wait for space
  task_wake_channel (pipe);
  irq_restore (flags);
  return total;
}

int32_t
pipe_write (struct pipe *pipe, const void *buf, uint32_t len)
{
  uint32_t flags = irq_save ();
  const char *in = buf;
  uint32_t written = 0;
  int32_t res = 0;
  while (written < len)
    {
      while (pipe->count == PIPE_SIZE && pipe->readers > 0
             && !task_running->killed)
        {
          task_block_on (pipe);
        }
      if (task_running->killed)
        {
          break;
        }
      if (pipe->readers == 0)
        {
          // nobody will ever read this, like writing to a closed socket
          res = -BROKEN_PIPE_ERROR;
          break;
        }
      uint32_t space = PIPE_SIZE - pipe->count;
      uint32_t total = len - written < space ? len - written : space;
      for (uint32_t i = 0; i < total; i++)
        {
          pipe->buffer[(pipe->head + pipe->count) % PIPE_SIZE] = in[written++];
          pipe->count++;
        }
      // readers may wait for data
      task_wake_channel (pipe);
    }
  irq_restore (flags);
  return written > 0 ? (int32_t)written : res;
}

static void
pipe_free_if_unused (struct pipe *pipe)
{
  if (pipe->readers == 0 && pipe->writers == 0)
    {
      free (pipe);
    }
}

void
pipe_close_reader (struct pipe *pipe)
{
  uint32_t flags = irq_save ();
  pipe->readers--;
  // blocked writers have to notice that nobody reads anymore
  task_wake_channel (pipe);
  pipe_free_if_unused (pipe);
  irq_restore (flags);
}

void
pipe_close_writer (struct pipe *pipe)
{
  uint32_t flags = irq_save ();
  pipe->writers--;
  // blocked readers get the end of the stream
  task_wake_channel (pipe);
  pipe_free_if_unused (pipe);
  irq_restore (flags);
}
