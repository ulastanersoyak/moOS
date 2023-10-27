#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/string/string.h"
#include "../memory/paging/page.h"
#include "./ascii/ascii.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kmem/kheap.h"

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
  system_page_dir = page_dir_init(IS_WRITABLE | IS_PRESENT | ACCESS_ALL);
  // tell processor where to find entry of system page directory
  switch_page_dir(system_page_dir->dir_entry);
  char* ptr = kcalloc(50);
  virtualize_mem(system_page_dir->dir_entry, (void*)0x1000, (uint32_t)ptr | ACCESS_ALL 
  | IS_PRESENT | IS_WRITABLE);
  // initialize paging for virtual memory and full system memory coverage
  enable_system_paging();
  char* ptr2 = (char*)0x1000;
  ptr2[0] = 'a';
  ptr2[1] = 'k';
  terminal_writestring(ptr2);
  terminal_writestring("\n");
  terminal_writestring(ptr);
  terminal_writestring("\n");
  // set everything before interrupts so system gets initialized without processor interrupts
  enable_interrupts();
}