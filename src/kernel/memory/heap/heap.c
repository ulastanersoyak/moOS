#include "heap.h"
#include "../../../drivers/screen/terminal.h"
#include "../../../libc/string/string.h"
#include "../../config.h"
#include <stdint.h>

int32_t
heap_desc_init (struct heap_desc *heap, void *start, void *end,
                struct heap_table *table)
{
  // check if start and end addresses allign with block size
  if (!((uint32_t)start % HEAP_BLOCK_SIZE == 0)
      && (!((uint32_t)end % HEAP_BLOCK_SIZE == 0)))
    {
      return -INVALID_ARG_ERROR;
    }
  // check if table contains same entries as declared
  if ((table->total_entries != ((size_t)((end - start) / HEAP_BLOCK_SIZE))))
    {
      return -INVALID_ARG_ERROR;
    }
  memset (heap, 0, sizeof (struct heap_desc));
  heap->addr = start;
  heap->table = table;
  size_t table_size = sizeof (HEAP_BLOCK_TABLE_ENTRY) * table->total_entries;
  memset (table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE,
          table_size); // set whole memory as free
  return OK;
}

// finds the available adjacent blocks that equal to requested amount
static uint32_t
find_free_block (struct heap_desc *desc, uint32_t total_blocks)
{
  uint32_t current_block = 0;
  int32_t start_block = -1;
  for (size_t i = 0; i < desc->table->total_entries; i++)
    {
      // if entries last bit is 1 it is in use, else its free
      if ((desc->table->entries[i] & 0x0f) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
          current_block = 0;
          start_block = -1;
          continue;
        }
      // mark the start of first available block
      if (start_block == -1)
        {
          start_block = i;
        }
      current_block++;
      if (current_block == total_blocks)
        {
          break;
        }
    }
  if (start_block == -1)
    {
      return -NO_MEMORY_ERROR;
    }
  return start_block;
}

static void
mark_taken_blocks (struct heap_desc *heap, uint32_t start_block,
                   uint32_t total_blocks)
{
  uint32_t end_block = (start_block + total_blocks) - 1;
  HEAP_BLOCK_TABLE_ENTRY entry
      = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
  // if heap has more than 1 block mark head as has next
  if (total_blocks > 1)
    {
      entry |= HEAP_BLOCK_HAS_NEXT;
    }
  for (size_t i = start_block; i <= end_block; i++)
    {
      // set the first block as the entry initialized as above
      heap->table->entries[i] = entry;
      entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
      // iterations dont use first bit override the entry as taken so next
      if (i != end_block - 1)
        {
          entry |= HEAP_BLOCK_HAS_NEXT;
          // as long as next block is not the end block mark has next bit
        }
    }
}

static void *
heap_malloc_blocks (struct heap_desc *heap, uint32_t total_blocks)
{
  void *address = 0;
  int32_t start_block = find_free_block (heap, total_blocks);
  if (start_block < 0)
    {
      return 0;
    }
  // get the address of heap memory start
  address = (heap->addr) + (HEAP_BLOCK_SIZE * start_block);
  mark_taken_blocks (heap, start_block, total_blocks);
  return address;
}

// wrapper for heap_malloc_blocks.
void *
heap_malloc (struct heap_desc *heap, size_t size)
{
  // align size to upper bound
  if (size % HEAP_BLOCK_SIZE > 0)
    {
      size = (size - (size % HEAP_BLOCK_SIZE)) + HEAP_BLOCK_SIZE;
    }
  uint32_t total_blocks = size / HEAP_BLOCK_SIZE;
  return heap_malloc_blocks (heap, total_blocks);
}

static void
heap_free_blocks (struct heap_desc *desc, void *addr)
{
  uint32_t block_number = ((uint32_t)(addr - desc->addr)) / HEAP_BLOCK_SIZE;
  for (size_t i = block_number; i < desc->table->total_entries; i++)
    {
      HEAP_BLOCK_TABLE_ENTRY entry = desc->table->entries[i];
      desc->table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
      if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
          break;
        }
    }
}

void
heap_free (struct heap_desc *heap, void *addr)
{
  heap_free_blocks (heap, addr);
}
