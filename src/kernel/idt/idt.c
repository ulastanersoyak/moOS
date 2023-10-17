#include "idt.h"
#include "../../drivers/screen/terminal.h"
#include "../../libc/string/string.h"
#include "../config.h"
#include "../io/io.h"

struct idt_entry idt[total_interrupts];
struct idtr_desc idtr_descriptor;

extern void int21h();
extern void no_intr();

extern void int21h_handler() {
  terminal_writestring("keyboard pressed");
  outb(0x20, 0x20);
}

void no_intr_handler() { outb(0x20, 0x20); }

void idt_zero() { terminal_writestring("divide by zero error"); }

void idt_init(void) {
  memset(idt, 0, sizeof(idt));
  idtr_descriptor.limit = ((sizeof(struct idt_entry) * total_interrupts) - 1);
  idtr_descriptor.base = (uint32_t)&idt[0];
  for (int i = 0; i < total_interrupts; i++) {
    idt_set(i, no_intr);
  }

  idt_set(0, idt_zero);
  idt_set(0x21, int21h);

  idt_load(&idtr_descriptor);
}

void idt_set(uint32_t interrupt_num, void *addr) {
  struct idt_entry *desc = &idt[interrupt_num];
  desc->isr_low = (uint32_t)addr & 0x0000ffff;
  desc->kernel_cs = kernel_code_seg;
  desc->zero = 0x00;
  desc->type_attr = 0xee;
  desc->isr_high = (uint32_t)addr >> 16;
}