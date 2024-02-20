#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/stdlib/stdlib.h"
#include "../memory/paging/page.h"
#include "ascii/ascii.h"
#include "idt/idt.h"
#include "kmem/kheap.h"

#include "../file_system/path_parser.h"

//TODO: READ.ME and all TODOs

static struct page_dir* system_page_dir = 0;

void kernel_main(void) {
  terminal_init();
  idt_init();
  kernel_heap_init();
  cowsay("booted without error! hopefully");
  // initialize systems page directory with given flags
  system_page_dir = page_dir_init(IS_WRITABLE | IS_PRESENT | ACCESS_ALL);
  // tell processor where to find entry of system page directory
  switch_page_dir(system_page_dir->dir_entry);
  // initialize paging for virtual memory and full system memory coverage
  enable_system_paging();
  enable_interrupts();
  struct path_root *root = get_path("0:/path/to/folder");
  // terminal_writestring(root->body->body_str);
  // terminal_writestring("\n");
  // terminal_writestring(root->body->next->body_str);
  // terminal_writestring("\n");
  // terminal_writestring(root->body->next->next->body_str);
  // terminal_writestring("\n");
  write_path(root);
}
