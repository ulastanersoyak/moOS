#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/string/string.h"
#include "../memory/paging/page.h"
#include "./ascii/ascii.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kmem/kheap.h"

static struct page_dir *kernel_paging_directory = 0;
void kernel_main(void) {
  terminal_init();
  kernel_heap_init();
  idt_init();
  kernel_paging_directory = page_dir_init(
      PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
  switch_page(get_dir(kernel_paging_directory));
  enable_paging();
  enable_interrupts();
}
