#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt_entry
{
  uint16_t isr_low;   // lower 16 bits of isr address
  uint16_t kernel_cs; // gdt segment selector that cpu will load into code
                      // segment before calling isr
  uint8_t zero;       // unused. set to zero
  uint8_t type_attr;  // gate type, dpl and p fields
  uint16_t isr_high;  // higher 16 bits of isr
} __attribute__ ((packed));

struct idtr_desc
{
  uint16_t limit;
  uint32_t base;
} __attribute__ ((packed));

// stack layout built by isr_common in idt.asm, lowest address first
struct interrupt_frame
{
  // data segment of the interrupted code
  uint32_t ds;
  // pushed by pushad
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  // pushed by the stub
  uint32_t vector;
  uint32_t error_code;
  // pushed by the cpu
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
  // pushed by the cpu only when interrupting ring 3
  uint32_t user_esp;
  uint32_t user_ss;
} __attribute__ ((packed));

// true when the interrupt arrived while running ring 3 code
#define FRAME_FROM_USER(frame) (((frame)->cs & 3) == 3)

// common c handler called by every exception and irq stub
void interrupt_handler (struct interrupt_frame *frame);

// initialize interrupt descriptor table
void idt_init (uint8_t verbose);

// add a new ring 0 interrupt gate to given index.
void idt_set (uint32_t interrupt_num, void *adr);

// add a new interrupt gate with given type and attributes to given index.
void idt_set_gate (uint32_t interrupt_num, void *addr, uint8_t type_attr);

// asm wrapper functions
void enable_interrupts (uint8_t verbose);
void disable_interrupts (void);

// asm functions
extern void idt_load (struct idtr_desc *ptr);
void enable_intr ();
void disable_intr ();

#endif // !IDT_H
