#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../libc/stdlib/stdlib.h"
#include "../memory/paging/page.h"
#include "ascii/ascii.h"
#include "idt/idt.h"
#include "kmem/kheap.h"
#include "../file_system/path_parser.h"
#include "../drivers/disk/disk.h"
#include "../drivers/disk/disk_stream.h"


//TODO: READ.ME and all TODOs
//TODO: custom memory tracker for leak detection

static struct page_dir* system_page_dir = 0;

void kernel_main(void) {
  terminal_init();
  idt_init();
  kernel_heap_init();
  // initialize systems page directory with given flags
  system_page_dir = page_dir_init(IS_WRITABLE | IS_PRESENT | ACCESS_ALL);
  // tell processor where to find entry of system page directory
  switch_page_dir(system_page_dir->dir_entry);
  // initialize paging for virtual memory and full system memory coverage
  enable_system_paging();
  init_main_master_disk();
  enable_interrupts();
  cowsay("booted without error! hopefully");
  struct disk_stream *stream= get_disk_stream(0);
  terminal_writeaddr(stream);
  terminal_writestring("\n");
  unsigned char c = 0;
  stream_seek(stream, 0x201);
  int32_t rs = disk_stream_read(stream, &c, 1);
  terminal_writeint((int)c);
  if(rs){}
}
