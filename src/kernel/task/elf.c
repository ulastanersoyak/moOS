#include "elf.h"
#include "../../libc/string/string.h"
#include "../config.h"

static int32_t
elf_validate_header (const struct elf32_header *header, uint32_t size)
{
  if (size < sizeof (struct elf32_header)
      || memcmp (header->ident, ELF_MAGIC, 4) != 0
      || header->ident[4] != ELF_CLASS_32 || header->ident[5] != ELF_DATA_LSB
      || header->type != ELF_TYPE_EXEC || header->machine != ELF_MACHINE_386
      || header->phentsize != sizeof (struct elf32_program_header))
    {
      return -INVALID_ARG_ERROR;
    }
  // program header table must lie inside the file
  uint32_t table_end
      = header->phoff + header->phnum * sizeof (struct elf32_program_header);
  if (table_end < header->phoff || table_end > size)
    {
      return -INVALID_ARG_ERROR;
    }
  return OK;
}

int32_t
elf_load (const uint8_t *buf, uint32_t size, uint32_t min_addr,
          uint32_t max_addr, ELF_COPY_FUNCTION copy, void *ctx,
          uint32_t *entry, uint32_t *image_end)
{
  const struct elf32_header *header = (const struct elf32_header *)buf;
  int32_t res = elf_validate_header (header, size);
  if (res < 0)
    {
      return res;
    }

  const struct elf32_program_header *ph
      = (const struct elf32_program_header *)(buf + header->phoff);
  uint32_t end = min_addr;
  uint8_t entry_loaded = 0;
  for (uint32_t i = 0; i < header->phnum; i++, ph++)
    {
      if (ph->type != ELF_PT_LOAD || ph->memsz == 0)
        {
          continue;
        }
      uint32_t seg_end = ph->vaddr + ph->memsz;
      uint32_t file_end = ph->offset + ph->filesz;
      if (ph->vaddr < min_addr || seg_end > max_addr || seg_end < ph->vaddr
          || ph->filesz > ph->memsz || file_end > size
          || file_end < ph->offset)
        {
          return -INVALID_ARG_ERROR;
        }
      uint8_t writable = (ph->flags & ELF_PF_W) != 0;
      // file contents, then map the zero filled rest of the segment
      res = copy (ctx, ph->vaddr, buf + ph->offset, ph->filesz, writable);
      if (res == OK && ph->memsz > ph->filesz)
        {
          res = copy (ctx, ph->vaddr + ph->filesz, 0, ph->memsz - ph->filesz,
                      writable);
        }
      if (res < 0)
        {
          return res;
        }
      if (header->entry >= ph->vaddr && header->entry < seg_end)
        {
          entry_loaded = 1;
        }
      if (seg_end > end)
        {
          end = seg_end;
        }
    }
  if (!entry_loaded)
    {
      return -INVALID_ARG_ERROR;
    }
  *entry = header->entry;
  *image_end = end;
  return OK;
}
