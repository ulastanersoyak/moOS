#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// written backwards because of little endiennes
struct idt_desc {
  uint16_t offset_1; // offset bits 0-15
  uint16_t selector;
  uint8_t zeros;     // unused. set to zero
  uint8_t type_attr; // gate type
  uint16_t offset_2; // offset bits 16-31
} __attribute__((packed));

struct idtr_desc {
  uint16_t limit; // size of descriptor table -1
  uint32_t base;  // base address of idt
} __attribute__((packed));

void idt_zero(void);
void idt_init(void);
void idt_set(uint32_t int_no, void *addr);

#endif // !IDT_H
