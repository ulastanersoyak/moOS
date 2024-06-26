#include "kheap.h"
#include "../../drivers/screen/terminal.h"
#include "../../kernel/memory/heap/heap.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"
#include "../config.h"

struct heap_desc kernel_heap;
struct heap_table kernel_heap_table;

void
kernel_heap_init (uint8_t verbose)
{
  uint32_t total_table_entries = HEAP_SIZE_BYTE / HEAP_BLOCK_SIZE;
  kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY *)(HEAP_TABLE_ADDRESS);
  kernel_heap_table.total_entries = total_table_entries;

  void *end_addr = (void *)(HEAP_ADDRESS + HEAP_SIZE_BYTE);
  int32_t res = heap_desc_init (&kernel_heap, (void *)(HEAP_ADDRESS), end_addr,
                                &kernel_heap_table);
  if (verbose)
    {
      printf ("heap init");
      if (res < 0)
        {
          init_ER ();
        }
      else
        {
          init_OK ();
        }
    }
}

void *
kmalloc (size_t size)
{
  return heap_malloc (&kernel_heap, size);
}

void *
kcalloc (size_t size)
{
  void *ptr = kmalloc (size);
  if (!ptr)
    {
      return 0;
    }
  memset (ptr, 0x00, size);
  return ptr;
}

void
kfree (void *addr)
{
  heap_free (&kernel_heap, addr);
}
