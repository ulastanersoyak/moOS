#include "idt.h"
#include "../../drivers/terminal.h"
#include "../../libc/string/string.h"
#include "../config.h"
#include "../io/io.h"

struct idt_desc idt_descriptors[total_interrupts];
struct idtr_desc idtr_descriptor;

extern void int21h();
extern void no_intr();
extern void int21h_handler() {
  terminal_writestring("keyboard pressed");
  outb(0x20, 0x20);
}

void no_intr_handler() { outb(0x20, 0x20); }

void idt_zero() { terminal_writestring("divide by zero error\n"); }

void idt_init(void) {
  memset(idt_descriptors, 0, sizeof(idt_descriptors));
  idtr_descriptor.limit = (sizeof(idt_descriptors) - 1);
  idtr_descriptor.base = (uint32_t)idt_descriptors;

  for (int i = 0; i < total_interrupts; i++) {
    idt_set(i, no_intr);
  }
  idt_set(0, idt_zero);
  idt_set(0x21, int21h);
  idt_load(&idtr_descriptor);
}

void idt_set(uint32_t interrupt_num, void *addr) {
  struct idt_desc *desc = &idt_descriptors[interrupt_num];
  desc->offset1 = (uint32_t)addr & 0x0000ffff;
  desc->selector = kernel_code_selector;
  desc->zero = 0x00;
  desc->type_attr = 0xee;
  desc->offset2 = (uint32_t)addr >> 16;
}
