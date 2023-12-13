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
  kernel_paging_directory = page_dir_init(
      PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
  switch_page(get_dir(kernel_paging_directory));
  enable_paging();
}
