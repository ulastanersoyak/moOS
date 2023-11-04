#include "kernel.h"
#include "../drivers/screen/terminal.h"
#include "../drivers/screen/vga.h"
#include "../libc/stdlib/stdlib.h"
#include "../memory/paging/page.h"
#include "ascii/ascii.h"
#include "idt/idt.h"
#include "kmem/kheap.h"
#include "../file_system/path_parser.h"
#include "../drivers/disk/disk.h"
#include "../drivers/disk/disk_stream.h"
#include "../libc/stdio/stdio.h"


//TODO: READ.ME and all TODOs
//TODO: custom memory tracker for leak detection

static struct page_dir* system_page_dir = 0;

void kernel_main(void) {
  uint8_t verbose = 0;
  terminal_init(verbose);
  idt_init(verbose);
  kernel_heap_init(verbose);
  // initialize systems page directory with given flags
  system_page_dir = page_dir_init(IS_WRITABLE | IS_PRESENT | ACCESS_ALL);
  // tell processor where to find entry of system page directory
  switch_page_dir(system_page_dir->dir_entry);
  // initialize paging for virtual memory and full system memory coverage
  enable_system_paging(verbose);
  init_main_master_disk(verbose);
  enable_interrupts(verbose);
  moose("VERSION 0.03. everything looks OK",light_red);
  struct disk_stream *stream= get_disk_stream(0);
  unsigned char c = 0;
  stream_seek(stream, 0x201);
  int32_t rs = disk_stream_read(stream, &c, 1);
  if(!rs){  
    printf("content of 0x201th bit in file -> %d\n",(int32_t)c);
  }
  printf("%d hex",0xbabe);
  printf("random numbers between 31-69\n");
  for(size_t i = 0; i <10; i++){
    uint32_t randrs = rand_range(31,69);
    printf("%d ",randrs);
  }
}
