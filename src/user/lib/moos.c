#include "moos.h"

void
print_int (int32_t num)
{
  char digits[10];
  uint32_t idx = 0;
  uint32_t mag = (uint32_t)num;
  if (num < 0)
    {
      putchar ('-');
      mag = -mag;
    }
  do
    {
      digits[idx++] = '0' + (mag % 10);
      mag /= 10;
    }
  while (mag > 0);
  while (idx > 0)
    {
      putchar (digits[--idx]);
    }
}

uint32_t
readline (char *buf, uint32_t max)
{
  // input from a file or pipe isnt typed, so it isnt echoed either
  int echo = isatty (STDIN);
  uint32_t len = 0;
  while (1)
    {
      int c = getchar ();
      // -1 is the end of the input, 4 is ctrl+d typed on the terminal
      if (c < 0 || c == '\n' || (echo && c == 4))
        {
          if (echo)
            {
              putchar ('\n');
            }
          break;
        }
      if (c == '\b')
        {
          if (len > 0)
            {
              len--;
              if (echo)
                {
                  putchar ('\b');
                }
            }
          continue;
        }
      if (c >= 32 && c <= 126 && len < max - 1)
        {
          buf[len++] = c;
          if (echo)
            {
              putchar (c);
            }
        }
    }
  buf[len] = '\0';
  return len;
}

uint32_t
strlen (const char *str)
{
  uint32_t len = 0;
  while (str[len])
    {
      len++;
    }
  return len;
}

int
strcmp (const char *a, const char *b)
{
  while (*a && *a == *b)
    {
      a++;
      b++;
    }
  return (unsigned char)*a - (unsigned char)*b;
}

void *
memcpy (void *dest, const void *src, uint32_t len)
{
  uint8_t *d = dest;
  const uint8_t *s = src;
  while (len--)
    {
      *d++ = *s++;
    }
  return dest;
}

void *
memset (void *dest, int c, uint32_t len)
{
  uint8_t *d = dest;
  while (len--)
    {
      *d++ = c;
    }
  return dest;
}

void
eprint (const char *str)
{
  write (STDERR, str, strlen (str));
}

void
print_padded (const char *str, uint32_t width)
{
  print (str);
  for (uint32_t len = strlen (str); len < width; len++)
    {
      putchar (' ');
    }
}

void
print_error (const char *what, int err)
{
  // error numbers from src/kernel/config.h
  static const char *reasons[] = {
    "error",
    "io error",
    "out of memory",
    "invalid argument",
    "invalid path",
    "too many open files",
    "invalid disk",
    "bad signature",
    "not permitted",
    "not implemented",
    "not a directory",
    "not found",
    "directory not empty",
    "already exists",
    "broken pipe",
  };
  int code = -err;
  eprint (what);
  eprint (": ");
  eprint (code > 0 && code < (int)(sizeof (reasons) / sizeof (reasons[0]))
              ? reasons[code]
              : "error");
  eprint ("\n");
}

// first fit allocator on top of sbrk. blocks are kept in address order so
// neighbouring free blocks can be merged
struct block
{
  uint32_t size; // payload bytes after the header
  uint32_t free;
  struct block *next;
};

#define ALIGN 8
#define HEADER_SIZE ((sizeof (struct block) + ALIGN - 1) & ~(ALIGN - 1))
#define MIN_SPLIT (HEADER_SIZE + ALIGN)

static struct block *heap_head = 0;
static struct block *heap_tail = 0;

static void *
block_payload (struct block *block)
{
  return (uint8_t *)block + HEADER_SIZE;
}

static struct block *
payload_block (void *ptr)
{
  return (struct block *)((uint8_t *)ptr - HEADER_SIZE);
}

void *
malloc (uint32_t size)
{
  if (size == 0)
    {
      return 0;
    }
  size = (size + ALIGN - 1) & ~(ALIGN - 1);
  for (struct block *block = heap_head; block; block = block->next)
    {
      if (!block->free || block->size < size)
        {
          continue;
        }
      // split off the unused tail if it is big enough to be useful
      if (block->size >= size + MIN_SPLIT)
        {
          struct block *rest
              = (struct block *)((uint8_t *)block_payload (block) + size);
          rest->size = block->size - size - HEADER_SIZE;
          rest->free = 1;
          rest->next = block->next;
          block->next = rest;
          block->size = size;
          if (heap_tail == block)
            {
              heap_tail = rest;
            }
        }
      block->free = 0;
      return block_payload (block);
    }
  struct block *block = sbrk (HEADER_SIZE + size);
  if (!block)
    {
      return 0;
    }
  block->size = size;
  block->free = 0;
  block->next = 0;
  if (heap_tail)
    {
      heap_tail->next = block;
    }
  else
    {
      heap_head = block;
    }
  heap_tail = block;
  return block_payload (block);
}

void
free (void *ptr)
{
  if (!ptr)
    {
      return;
    }
  payload_block (ptr)->free = 1;
  // merge runs of adjacent free blocks
  for (struct block *block = heap_head; block; block = block->next)
    {
      while (block->free && block->next && block->next->free)
        {
          if (heap_tail == block->next)
            {
              heap_tail = block;
            }
          block->size += HEADER_SIZE + block->next->size;
          block->next = block->next->next;
        }
    }
}

void *
calloc (uint32_t count, uint32_t size)
{
  uint32_t total = count * size;
  if (size && total / size != count)
    {
      return 0;
    }
  void *ptr = malloc (total);
  if (ptr)
    {
      memset (ptr, 0, total);
    }
  return ptr;
}

void *
realloc (void *ptr, uint32_t size)
{
  if (!ptr)
    {
      return malloc (size);
    }
  struct block *block = payload_block (ptr);
  if (block->size >= size)
    {
      return ptr;
    }
  void *bigger = malloc (size);
  if (bigger)
    {
      memcpy (bigger, ptr, block->size);
      free (ptr);
    }
  return bigger;
}
