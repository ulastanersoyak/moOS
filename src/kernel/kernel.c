#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/string/string.h"
#include "../memory/paging/page.h"
#include "./ascii/ascii.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kmem/kheap.h"

// kernels paging directory. contains 1024 page entries which contains 1024
// page, each page containing 4kib memory. thus resulting in 4gb virtual memory
static struct page_dir *kernel_paging_directory = 0;

void kernel_main(void) {
  terminal_init();
  idt_init();
  enable_interrupts();
  kernel_heap_init();
  kernel_paging_directory =
      page_dir_init(IS_WRITABLE | IS_PRESENT | ACCESS_FROM_ALL);
  switch_page_dir(get_current_dir(kernel_paging_directory));
  char *ptr = kcalloc(4096);
  enable_paging();
  set_page(kernel_paging_directory->dir_entry, (void *)0x1000,
           (uint32_t)ptr | ACCESS_FROM_ALL | IS_PRESENT | ACCESS_FROM_ALL);
  char *ptr2 = (char *)0x1000;
  ptr2[0] = 'a';
  ptr2[1] = 'b';
  ptr2[2] = 'c';
  ptr2[3] = 'd';
  ptr2[4] = 'e';
  terminal_writestring(ptr);
  terminal_writestring("\n");
  terminal_writestring(ptr2);
  terminal_writestring("\n");
}
