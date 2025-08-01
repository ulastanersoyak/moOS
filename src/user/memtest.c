#include "lib/moos.h"

#define BLOCKS 64

// exercises the user heap: allocations of many sizes, freeing every other
// block, reuse of freed memory and growing with realloc
int
main (void)
{
  uint8_t *blocks[BLOCKS];
  uint32_t start = (uint32_t)sbrk (0);
  for (int i = 0; i < BLOCKS; i++)
    {
      uint32_t size = 16 + i * 97;
      blocks[i] = malloc (size);
      if (!blocks[i])
        {
          print ("memtest: malloc failed\n");
          return 1;
        }
      memset (blocks[i], i, size);
    }
  for (int i = 0; i < BLOCKS; i += 2)
    {
      free (blocks[i]);
    }
  // odd blocks must be untouched
  for (int i = 1; i < BLOCKS; i += 2)
    {
      for (uint32_t j = 0; j < 16 + i * 97u; j++)
        {
          if (blocks[i][j] != i)
            {
              print ("memtest: heap corrupted\n");
              return 1;
            }
        }
    }
  uint32_t grown = (uint32_t)sbrk (0);
  // freed holes are reused, the heap must not grow for these
  for (int i = 0; i < BLOCKS; i += 2)
    {
      blocks[i] = malloc (16);
    }
  if ((uint32_t)sbrk (0) != grown)
    {
      print ("memtest: freed memory was not reused\n");
      return 1;
    }
  char *text = malloc (4);
  memcpy (text, "moo", 4);
  text = realloc (text, 100000);
  if (!text || strcmp (text, "moo") != 0)
    {
      print ("memtest: realloc lost data\n");
      return 1;
    }
  for (int i = 0; i < BLOCKS; i++)
    {
      free (blocks[i]);
    }
  free (text);
  print ("memtest: ok, heap grew by ");
  print_int (((uint32_t)sbrk (0) - start) / 1024);
  print (" kb\n");
  return 0;
}
