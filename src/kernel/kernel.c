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

  // TODO FIX BLOCK BUG
  char *ptr = (char *)kmalloc(5000);
  char *ptr2 = (char *)kmalloc(500);
  ptr[0] = 'p';
  ptr[1] = 't';
  ptr[2] = 'r';
  ptr[3] = '1';
  ptr[4] = '\n';

  ptr2[0] = 'p';
  ptr2[1] = 't';
  ptr2[2] = 'r';
  ptr2[3] = '2';
  ptr2[4] = '\n';

  char *ptr3 = (char *)kmalloc(5000);
  ptr3[0] = 'p';
  ptr3[1] = 't';
  ptr3[2] = 'r';
  ptr3[3] = '3';
  ptr3[4] = '\n';

  terminal_writeaddr(ptr);
  terminal_writestring("\n");

  terminal_writeaddr(ptr2);
  terminal_writestring("\n");

  terminal_writeaddr(ptr3);
  terminal_writestring("\n");
  //  shrigma();
}
