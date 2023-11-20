#include "idt.h"
#include "../drivers/terminal.h"
#include "../drivers/vga.h"
#include "../kernel/config.h"      // total_interrupts
#include "../libc/string/string.h" // memset

struct idt_desc idt_descriptors[total_interrupts];

struct idtr_desc idtr_desciptor;

void idt_load(struct idtr_desc *ptr) {
  __asm__ volatile("push %%ebp\n"
                   "mov %%ebp, %%esp\n"
                   "mov %%ebx, %0\n"
                   "lidt (%%ebx)\n"
                   "pop %%ebp\n"
                   :
                   : "r"(ptr));
}

void idt_zero(void) {
  terminal_setcolour(red);
  terminal_writestring("divide by zero error\n");
  terminal_setcolour(black);
}

void idt_init(void) {
  memset(idt_descriptors, 0, sizeof(idt_descriptors));
  idtr_desciptor.limit = sizeof(idt_descriptors) - 1;
  idtr_desciptor.base = (uint32_t)idt_descriptors;
  idt_set(0, idt_zero);
  // load idt
  idt_load(&idtr_desciptor);
}

void idt_set(uint32_t int_no, void *addr) {
  struct idt_desc *desc = &idt_descriptors[int_no];
  desc->offset_1 = (uint32_t)addr & 0x0000FFFF;
  desc->selector = kernel_code_selector;
  desc->zeros = 0x00;
  desc->type_attr = 0xEE;
  desc->offset_2 = (uint32_t)addr >> 16;
}
