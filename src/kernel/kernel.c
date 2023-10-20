#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/string/string.h"
#include "./ascii/ascii.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kmem/kheap.h"

void kernel_main(void) {
  terminal_init();
  kernel_heap_init();
  idt_init();
  void *ptr1 = kmalloc(500);
  void *ptr2 = kmalloc(50);
  terminal_writeaddr(ptr1);
  terminal_writestring("\n");
  terminal_writeaddr(ptr2);
  terminal_writestring("\n");
  kfree(ptr1);
  void *ptr3 = kmalloc(50);
  terminal_writeaddr(ptr3);
}
