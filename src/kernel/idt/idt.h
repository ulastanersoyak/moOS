#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt_desc {
  uint16_t offset1; // offset bits 0-15. address of the entry point in interrupt
                    // service routine
  uint16_t selector; // a segment selector with multiple fields. must point to a
                     // valid segment in gdt
  uint8_t zero;      // unused
  uint8_t type_attr; // gate type, dpl and p fields
  uint16_t offset2;  // offset bits 15-31
} __attribute__((packed));

struct idtr_desc {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

// initialize interrupt descriptor table
void idt_init(void);

// add a new interrupt to given index.
void idt_set(uint32_t interrupt_num, void *adr);

// dont use. for loading idt with asm
extern void idt_load(struct idtr_desc *ptr);

#endif // !IDT_H
