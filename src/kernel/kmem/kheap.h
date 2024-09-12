#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

void kernel_heap_init (uint8_t verbose);

// allocates heap memory aligned to upper block limit
void *kmalloc (size_t size);

// allocates heap memory aligned to upper block limit and sets all to 0
void *kcalloc (size_t);

// marks given address as free
void kfree (void *addr);

#endif // !KHEAP_H
