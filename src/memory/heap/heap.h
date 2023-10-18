#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0X01 // 0b00000001

#define HEAP_BLOCK_TABLE_ENTRY_FREE 0X00 // 0b00000000

#define HEAP_BLOCK_HAS_NEXT                                                    \
  0b10000000 // bitmap for checking next block taken or not

#define HEAP_BLOCK_IS_FIRST 0b01000000 // bitmap for checking block free or not

typedef unsigned char
    HEAP_BLOCK_TABLE_ENTRY; // entries for each block. contains information
                            // about block itself as a bitmap

struct heap_table {
  HEAP_BLOCK_TABLE_ENTRY *entries;
  size_t total_entries;
};

struct heap_desc {
  struct heap_table *table; // array of entries
  void *addr;               // start address of heap
};

// takes pointers to start and end to heap area, pointer to a heap table and
// pointer to heap. returns 0 if successfull
int32_t heap_desc_init(struct heap_desc *heap, void *start, void *end,
                       struct heap_table *table);

void *heap_malloc(struct heap_desc *heap, size_t size);
void heap_free(struct heap_desc *heap, void *ptr);
#endif // !HEAP_H
