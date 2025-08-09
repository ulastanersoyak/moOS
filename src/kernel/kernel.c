#include "../drivers/disk/disk.h"
#include "../drivers/screen/terminal.h"
#include "../drivers/timer/pit.h"
#include "../libc/stdio/stdio.h"
#include "../libc/stdlib/stdlib.h"
#include "../libc/string/string.h"
#include "./memory/paging/page.h"
#include "config.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "kmem/kheap.h"
#include "multiboot.h"
#include "shell/shell.h"
#include "task/task.h"
#include "task/tss.h"
#include <stdint.h>

// TODO: vga driver             [x]
// TODO: basic idt              [x]
// TODO: heap allocator         [x]
// TODO: paging                 [x]
// TODO: disk io                [x]
// TODO: full idt               [x]
// TODO: keyboard driver        [x]
// TODO: vfs                    [x]
// TODO: file system            [x]
// TODO: shell                  [x]
// TODO: userland               [x]
// TODO: scheduler              [x]
// TODO: rtos                   [ ]

#define IS_RTOS (0)

// reads test files from the disk image at boot. see makefile for their content
#define RUN_SELF_TESTS (1)

// gdt and tss must outlive kernel_main. the cpu keeps reading them from memory
// (e.g. iret reloads cs from the gdt) and interrupts reuse the stack once
// kernel_main returns
struct tss tss;
static struct gdt_raw gdt_real[TOTAL_GDT_SEGMENTS];

#if (RUN_SELF_TESTS == 1 && IS_RTOS != 1)
// reads a whole file and checks it against expected content.
// expected == 0 checks lines of the form "line NNNN\n" instead
static void
fs_test_file (const char *path, const char *expected)
{
  printf ("fs test %s", path);
  int fd = fopen (path, "r");
  if (!fd)
    {
      printf (" (open failed)");
      init_ER ();
      return;
    }
  struct file_stat stat;
  if (fstat (fd, &stat) != OK)
    {
      printf (" (stat failed)");
      init_ER ();
      fclose (fd);
      return;
    }
  char *buf = calloc (stat.filesize + 1);
  if (fread (buf, stat.filesize, 1, fd) != 1)
    {
      printf (" (read failed)");
      init_ER ();
      free (buf);
      fclose (fd);
      return;
    }
  int ok = 1;
  if (expected)
    {
      ok = strlen (expected) == stat.filesize
           && memcmp (buf, expected, stat.filesize) == 0;
    }
  else
    {
      char line[11];
      for (uint32_t i = 0; i * 10 < stat.filesize && ok; i++)
        {
          uint32_t n = i + 1;
          memcpy (line, "line 0000\n", 11);
          for (int d = 8; d >= 5; d--, n /= 10)
            {
              line[d] = '0' + (n % 10);
            }
          ok = memcmp (buf + i * 10, line, 10) == 0;
        }
    }
  printf (" <%d bytes>", stat.filesize);
  ok ? init_OK () : init_ER ();
  free (buf);
  fclose (fd);
}
#endif

// name of the bootloader, copied out of the multiboot info before the
// memory it lives in gets reused
static char boot_loader_name[48] = "moOS bootloader";

static void
read_boot_info (uint32_t boot_magic, uint32_t boot_info)
{
  if (boot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
      return;
    }
  struct multiboot_info *info = (struct multiboot_info *)boot_info;
  if (info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
    {
      const char *name = (const char *)info->boot_loader_name;
      uint32_t i = 0;
      for (; name[i] && i < sizeof (boot_loader_name) - 1; i++)
        {
          boot_loader_name[i] = name[i];
        }
      boot_loader_name[i] = '\0';
    }
  else
    {
      strcpy (boot_loader_name, "unknown multiboot loader");
    }
}

// boot_magic and boot_info come from a multiboot loader (eax and ebx), both
// are 0 when booted by the moOS bootloader
void
kernel_main (uint32_t boot_magic, uint32_t boot_info)
{
  uint8_t verbose = 1;

  read_boot_info (boot_magic, boot_info);
  terminal_init (verbose);
  if (verbose)
    {
      printf ("booted by %s", boot_loader_name);
      init_OK ();
    }

  // segments specified in bootloader written in c
  struct gdt gdt[TOTAL_GDT_SEGMENTS] = {
    { .base = 0x00, .limit = 0x00, .type = 0x00 },       // null segment
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0x9A }, // kernel code segment
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0x92 }, // kernel data segment
    // user segments has all system memory available because of paging
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0xFA }, // user code segment
    { .base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF2 }, // user data segment
    { .base = (uint32_t)&tss,
      .limit = sizeof (tss) - 1,
      .type = 0x89 }, // tss segment, ring 0 available 32 bit tss
  };
  memset (gdt_real, 0x00, sizeof (gdt_real));
  gdt_to_raw (gdt_real, gdt, TOTAL_GDT_SEGMENTS);
  gdt_raw_load (gdt_real, sizeof (gdt_real) - 1);
  if (verbose)
    {
      printf ("gdt init");
      init_OK ();
    }

  idt_init (verbose);

  memset (&tss, 0x00, sizeof (tss));
  // esp0 is set to the kernel stack of each user task when it is scheduled
  tss.ss0 = KERNEL_DATA_SEG;
  tss.iopb = sizeof (tss); // no io permission bitmap
  tss_load (TSS_SEG);
  if (verbose)
    {
      printf ("tss init");
      init_OK ();
    }

  kernel_heap_init (verbose);

#if (IS_RTOS != 1)
  fs_init (verbose);
  struct page_dir *system_page_dir = 0;
  // kernel pages are supervisor only, user tasks get their own pages
  system_page_dir = page_dir_init (IS_WRITABLE | IS_PRESENT);
  // makes processor find entry of system page directory
  switch_page_dir (system_page_dir->dir_entry);
  // initializes paging for virtual memory and full system memory coverage
  enable_system_paging (verbose);
  init_main_master_disk (verbose);
#if (RUN_SELF_TESTS == 1)
  fs_test_file ("0:/fs_test.txt", "file sys text\n");
  fs_test_file ("0:/dir/long.txt", 0);
#endif
  task_init (system_page_dir, verbose);
#endif
  pit_init (verbose);
  enable_interrupts (verbose);
#if (IS_RTOS != 1)
  shell_run ();
#endif
}
