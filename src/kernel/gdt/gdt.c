#include "gdt.h"
#include "../kpanic.h"
#include <stdint.h>

void
encode_gdt_entry (uint8_t *target, struct gdt source)
{
  if ((source.limit > 65536) && (source.limit & 0xFFFF) != 0xFFFF)
    {
      panic ("encode_gdt_entry: invalid argument");
    }
  target[6] = 0x40;
  if (source.limit > 655536)
    {
      source.limit = source.limit >> 12;
      target[6] = 0xC0;
    }
  // encodes limit
  target[0] = source.limit & 0xFF;
  target[1] = (source.limit >> 8) & 0xFF;
  target[6] = (source.limit >> 16) & 0x0F;

  // encodes base
  target[2] = source.base & 0xFF;
  target[3] = (source.base >> 8) & 0xFF;
  target[4] = (source.base >> 16) & 0xFF;
  target[7] = (source.base >> 24) & 0xFF;
  target[5] = source.type;
}

void
gdt_to_raw (struct gdt_raw *raw, struct gdt *gdt, uint32_t total_entry)
{
  for (uint32_t i = 0; i < total_entry; i++)
    {
      encode_gdt_entry ((uint8_t *)&raw[i], gdt[i]);
    }
}
