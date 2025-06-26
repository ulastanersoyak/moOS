#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF_MAGIC                                                             \
  "\x7f"                                                                      \
  "ELF"
#define ELF_CLASS_32 1
#define ELF_DATA_LSB 1
#define ELF_TYPE_EXEC 2
#define ELF_MACHINE_386 3
#define ELF_PT_LOAD 1
#define ELF_PF_W 0x2

struct elf32_header
{
  uint8_t ident[16];
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint32_t entry;
  uint32_t phoff; // program header table offset
  uint32_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
} __attribute__ ((packed));

struct elf32_program_header
{
  uint32_t type;
  uint32_t offset;
  uint32_t vaddr;
  uint32_t paddr;
  uint32_t filesz; // bytes stored in the file
  uint32_t memsz;  // bytes in memory, the rest is zeroed (bss)
  uint32_t flags;
  uint32_t align;
} __attribute__ ((packed));

// copies len bytes of src to user address vaddr, mapping pages as needed.
// src may be 0 to only map (zeroed) memory
typedef int32_t (*ELF_COPY_FUNCTION) (void *ctx, uint32_t vaddr,
                                      const void *src, uint32_t len,
                                      uint8_t writable);

// validates an i386 executable in buf and loads every PT_LOAD segment that
// lies within [min_addr, max_addr). sets entry and the end of the highest
// segment. returns 0 or a negative error
int32_t elf_load (const uint8_t *buf, uint32_t size, uint32_t min_addr,
                  uint32_t max_addr, ELF_COPY_FUNCTION copy, void *ctx,
                  uint32_t *entry, uint32_t *image_end);

#endif // !ELF_H
