#include "handle.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../drivers/screen/terminal.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/stdlib/stdlib.h"
#include "../config.h"
#include "pipe.h"
#include "task.h"

#define TERMINAL_LINE_MAX 256

static struct handle terminal = { .type = HANDLE_TERMINAL, .refs = 1 };

// a line read from the keyboard but not consumed by read yet
static char terminal_line[TERMINAL_LINE_MAX];
static uint32_t terminal_line_pos = 0;
static uint32_t terminal_line_len = 0;

struct handle *
handle_terminal (void)
{
  return &terminal;
}

static struct handle *
handle_new (enum handle_type type)
{
  struct handle *handle = calloc (sizeof (struct handle));
  if (handle)
    {
      handle->type = type;
      handle->refs = 1;
    }
  return handle;
}

struct handle *
handle_new_file (int file_fd)
{
  struct handle *handle = handle_new (HANDLE_FILE);
  if (handle)
    {
      handle->file_fd = file_fd;
    }
  return handle;
}

int32_t
handle_new_pipe (struct handle **read_end, struct handle **write_end)
{
  struct pipe *pipe = pipe_new ();
  *read_end = handle_new (HANDLE_PIPE_READ);
  *write_end = handle_new (HANDLE_PIPE_WRITE);
  if (!pipe || !*read_end || !*write_end)
    {
      free (pipe);
      free (*read_end);
      free (*write_end);
      return -NO_MEMORY_ERROR;
    }
  (*read_end)->pipe = pipe;
  (*write_end)->pipe = pipe;
  return OK;
}

struct handle *
handle_ref (struct handle *handle)
{
  handle->refs++;
  return handle;
}

void
handle_close (struct handle *handle)
{
  if (handle == &terminal || --handle->refs > 0)
    {
      return;
    }
  switch (handle->type)
    {
    case HANDLE_FILE:
      fclose (handle->file_fd);
      break;
    case HANDLE_PIPE_READ:
      pipe_close_reader (handle->pipe);
      break;
    case HANDLE_PIPE_WRITE:
      pipe_close_writer (handle->pipe);
      break;
    case HANDLE_TERMINAL:
      break;
    }
  free (handle);
}

// fills terminal_line with one line typed by the user. returns 0 on ctrl+d
// at the start of a line, -1 if the task got killed
static int32_t
terminal_read_line (void)
{
  terminal_line_pos = 0;
  terminal_line_len = 0;
  while (1)
    {
      int32_t key = task_wait_key ();
      if (key < 0)
        {
          return -1;
        }
      if (key == KEY_CTRL_D)
        {
          // ends the input, or the line without a newline
          return terminal_line_len;
        }
      if (key == '\b')
        {
          if (terminal_line_len > 0)
            {
              terminal_line_len--;
              terminal_putchar ('\b');
            }
          continue;
        }
      if (key == '\n' || (key >= 32 && key < 127))
        {
          if (terminal_line_len < TERMINAL_LINE_MAX)
            {
              terminal_line[terminal_line_len++] = key;
              terminal_putchar (key);
            }
          if (key == '\n')
            {
              return terminal_line_len;
            }
        }
    }
}

static int32_t
terminal_read (void *buf, uint32_t len)
{
  if (terminal_line_pos == terminal_line_len)
    {
      int32_t res = terminal_read_line ();
      if (res <= 0)
        {
          return res;
        }
    }
  uint32_t left = terminal_line_len - terminal_line_pos;
  uint32_t total = len < left ? len : left;
  char *out = buf;
  for (uint32_t i = 0; i < total; i++)
    {
      out[i] = terminal_line[terminal_line_pos++];
    }
  return total;
}

int32_t
handle_read (struct handle *handle, void *buf, uint32_t len)
{
  switch (handle->type)
    {
    case HANDLE_TERMINAL:
      return terminal_read (buf, len);
    case HANDLE_FILE:
      // fread with 1 byte members returns the bytes read, 0 at the end
      return len ? fread (buf, 1, len, handle->file_fd) : 0;
    case HANDLE_PIPE_READ:
      return pipe_read (handle->pipe, buf, len);
    case HANDLE_PIPE_WRITE:
      break;
    }
  return -INVALID_ARG_ERROR;
}

int32_t
handle_write (struct handle *handle, const void *buf, uint32_t len)
{
  switch (handle->type)
    {
    case HANDLE_TERMINAL:
      terminal_write (buf, len);
      return len;
    case HANDLE_FILE:
      return len ? fwrite (buf, 1, len, handle->file_fd) : 0;
    case HANDLE_PIPE_WRITE:
      return pipe_write (handle->pipe, buf, len);
    case HANDLE_PIPE_READ:
      break;
    }
  return -INVALID_ARG_ERROR;
}

int32_t
handle_getchar (struct handle *handle)
{
  if (handle->type == HANDLE_TERMINAL)
    {
      return task_wait_key ();
    }
  unsigned char c;
  return handle_read (handle, &c, 1) == 1 ? c : -1;
}
