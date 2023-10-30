#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/string/string.h"
#include "../libc/stdlib/stdlib.h"
#include "../memory/paging/page.h"
#include "./ascii/ascii.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kmem/kheap.h"
#include "../drivers/disk/disk.h"

// system paging directory pointer.
static struct page_dir* system_page_dir = 0;

void kernel_main(void) {
  // initialize terminal so ascii chars can be printed
  terminal_init();
  // initialize idt so if an interrupt happens system wont panic
  idt_init();
  // initialize system heap for further use
  kernel_heap_init();
  // initialize systems paging directory
  cowsay("booted without error! hopefully");
  // initialize systems page directory with given flags
  system_page_dir = page_dir_init(IS_WRITABLE | IS_PRESENT | ACCESS_ALL);
  // tell processor where to find entry of system page directory
  switch_page_dir(system_page_dir->dir_entry);
  // initialize paging for virtual memory and full system memory coverage
  enable_system_paging();
  // set everything before interrupts so system gets initialized without processor interrupts
  enable_interrupts();
  char *ptr = "1text234text56";
  int32_t rs = atoi(ptr);
  terminal_writeint(rs);
}
