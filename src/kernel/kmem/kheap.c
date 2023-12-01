#include "kheap.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"
#include "../../memory/heap/heap.h"
#include "../config.h"
struct heap_desc kernel_heap;
struct heap_table kernel_heap_table;

void kernel_heap_init(void) {
  terminal_writestring("kernel heap initializing");
  uint32_t total_table_entries = HEAP_SIZE_BYTE / HEAP_BLOCK_SIZE;
  kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY *)(HEAP_ADDRESS);
  kernel_heap_table.total_entries = total_table_entries;

  void *end_addr = (void *)(HEAP_ADDRESS + HEAP_SIZE_BYTE);
  int32_t res = heap_desc_init(&kernel_heap, (void *)(HEAP_ADDRESS), end_addr,
                               &kernel_heap_table);
  if (res < 0) {
    terminal_setcolour(red);
    terminal_writestring(" [FAILED]\n");
    terminal_setcolour(white);
  } else {

    terminal_setcolour(green);
    terminal_writestring(" [OK]\n");
    terminal_setcolour(white);
  }
}

void *kmalloc(size_t size) { return heap_malloc(&kernel_heap, size); }
