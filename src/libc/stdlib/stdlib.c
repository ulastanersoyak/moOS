#include "stdlib.h"
#include "../../kernel/config.h"

#include <stdint.h>

// TODO
void *malloc(size_t size) {
  // align size to upper bound
  if (size % HEAP_BLOCK_SIZE > 0) {
    size = (size - (size % HEAP_BLOCK_SIZE)) + HEAP_BLOCK_SIZE;
  }
  uint32_t total_blocks = size / HEAP_BLOCK_SIZE;
}
void *calloc(size_t size);
void free(void *ptr);
