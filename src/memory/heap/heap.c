#include "heap.h"
#include "../../kernel/config.h"
#include "../../libc/string/string.h"

int32_t heap_desc_init(struct heap_desc *heap, void *start, void *end,
                       struct heap_table *table) {

  uint32_t rs = OK;

  // check if start and end addresses allign with block size
  if (!((uint32_t)start % HEAP_BLOCK_SIZE == 0) &&
      !((uint32_t)end % HEAP_BLOCK_SIZE == 0)) {
    rs = -INVALID_ARG_ERROR;
    return rs;
  }

  // check if table contains same entries as declared
  if (!(table->total_entries != ((size_t)(end - start) / HEAP_BLOCK_SIZE))) {
    rs = -INVALID_ARG_ERROR;
    return rs;
  }

  memset(heap, 0, sizeof(struct heap_desc));
  heap->addr = start;
  heap->table = table;

  size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total_entries;
  memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE,
         table_size); // mark up whole memory as free and usable.

  return rs;
}
