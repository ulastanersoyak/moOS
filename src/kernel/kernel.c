#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/string/string.h"
#include "./ascii/ascii.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kmem/kheap.h"
void test_maloc(char *ptr) {
  for (int i = 0; i < 5; i++) {
    terminal_putchar(ptr[i]);
  }
}
void kernel_main(void) {
  terminal_initialize();
  kernel_heap_init();
  idt_init();
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

  test_maloc(ptr);
  test_maloc(ptr2);
  //  shrigma();
}
