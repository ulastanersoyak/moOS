#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>

void kernel_heap_init(void);

// marks given address as used and returns the pointer to the start of heap
void *kmalloc(size_t size);

void *kcalloc(size_t);

// marks given address as free
void kfree(void *addr);

#endif // !KHEAP_H
