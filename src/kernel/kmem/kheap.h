#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>

void kernel_heap_init(void);
void *kmalloc(size_t size);

#endif // !KHEAP_H
