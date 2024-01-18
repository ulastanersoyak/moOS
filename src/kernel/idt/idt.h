#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt_entry {
  uint16_t isr_low;   // lower 16 bits of isr address
  uint16_t kernel_cs; // gdt segment selector that cpu will load into code segment before calling isr
  uint8_t zero;       // unused. set to zero
  uint8_t type_attr;  // gate type, dpl and p fields
  uint16_t isr_high;  // higher 16 bits of isr
} __attribute__((packed));

struct idtr_desc {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

// initialize interrupt descriptor table
void idt_init(void);

// add a new interrupt to given index.
void idt_set(uint32_t interrupt_num, void *adr);
void enable_interrupts(void);
void disble_interrupts(void);

extern void idt_load(struct idtr_desc *ptr);

void enable_intr();
void disable_intr();

#endif // !IDT_H
