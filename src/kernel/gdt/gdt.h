#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdt_raw
{
  uint16_t segment;
  uint16_t base_first;
  uint8_t base;
  uint8_t access;
  uint8_t high_flags;
  uint8_t base24_31bits;
};

struct gdt
{
  uint32_t base;
  uint32_t limit;
  uint8_t type;
};

// loads gdt and reloads every segment register with the kernel segments.
// size is the gdt limit, which is one less than its size in bytes
void gdt_raw_load (struct gdt_raw *raw, uint32_t size);

// transforms structured gdt to format expected by processor
void gdt_to_raw (struct gdt_raw *raw, struct gdt *gdt, uint32_t total_entry);

#endif // !GDT_H
