#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// value of eax when a multiboot 1 loader jumps to the kernel
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// multiboot_info.flags bits
#define MULTIBOOT_INFO_MEMORY (1 << 0)
#define MULTIBOOT_INFO_BOOT_LOADER_NAME (1 << 9)

// start of the info structure passed in ebx, only the fields moOS reads
struct multiboot_info
{
  uint32_t flags;
  uint32_t mem_lower; // kb of memory below 1 mb
  uint32_t mem_upper; // kb of memory above 1 mb
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t syms[4];
  uint32_t mmap_length;
  uint32_t mmap_addr;
  uint32_t drives_length;
  uint32_t drives_addr;
  uint32_t config_table;
  uint32_t boot_loader_name; // pointer to a null terminated string
} __attribute__ ((packed));

#endif // !MULTIBOOT_H
