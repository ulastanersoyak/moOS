#include "../drivers/disk/disk.h"
#include "../drivers/screen/terminal.h"
#include "../libc/string/string.h"
#include "./memory/paging/page.h"
#include "config.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "kmem/kheap.h"
#include "task/tss.h"
#include <stdint.h>

// TODO: vga driver             [x]
// TODO: basic idt              [x]
// TODO: heap allocator         [x]
// TODO: paging                 [x]
// TODO: disk io                [x]
// TODO: full idt               [ ]
// TODO: keyboard driver        [ ]
// TODO: vfs                    [ ]
// TODO: file system            [ ]
// TODO: rtos                   [ ]
// TODO: userland               [ ]

#define IS_RTOS (0)

void
kernel_main (void)
{
  uint8_t verbose = 1;

  terminal_init (verbose);

  struct tss tss;
  struct gdt_raw gdt_real[TOTAL_GDT_SEGMENTS];
  // segments specified in bootloader written in c
  struct gdt gdt[TOTAL_GDT_SEGMENTS] = {
    { .base = 0x00, .limit = 0x00, .type = 0x00 },       // null segment
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0x9A }, // kernel code segment
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0x92 }, // kernel data segment
    // user segments has all system memory available because of paging
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF8 }, // user code segment
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF8 }, // user data segment
    { .base = (uint32_t)&tss,
      .limit = sizeof (tss),
      .type = 0xE9 }, // tss segment
  };
  memset (gdt_real, 0x00, sizeof (gdt_real));
  gdt_to_raw (gdt_real, gdt, TOTAL_GDT_SEGMENTS);
  gdt_raw_load (gdt_real, sizeof (gdt_real));

  idt_init (verbose);

  memset (&tss, 0x00, sizeof (tss));
  tss.esp0 = 0x600000; // kernels stack
  tss.ss0 = KERNEL_DATA_SEG;
  tss_load (0x28); // offset in gdt

  kernel_heap_init (verbose);

#if (IS_RTOS != 1)
  fs_init (verbose);
  struct page_dir *system_page_dir = 0;
  system_page_dir = page_dir_init (IS_WRITABLE | IS_PRESENT | ACCESS_ALL);
  // makes processor find entry of system page directory
  switch_page_dir (system_page_dir->dir_entry);
  // initializes paging for virtual memory and full system memory coverage
  enable_system_paging (verbose);
  init_main_master_disk (verbose);
#endif
  enable_interrupts (verbose);
}
