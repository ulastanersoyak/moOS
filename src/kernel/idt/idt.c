#include "idt.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../drivers/screen/terminal.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"
#include "../config.h"
#include "../io/io.h"
#include "../kpanic.h"

// idt table array
struct idt_entry idt[TOTAL_INTERRUPTS];

// idt tables descriptor that contains address and length of idt
struct idtr_desc idtr_descriptor;

// addresses of the asm stubs for exceptions and pic irqs
extern uint32_t isr_stub_table[TOTAL_ISR_STUBS];
extern void no_intr ();

static const char *exception_names[32] = {
  "divide by zero",
  "debug",
  "non maskable interrupt",
  "breakpoint",
  "overflow",
  "bound range exceeded",
  "invalid opcode",
  "device not available",
  "double fault",
  "coprocessor segment overrun",
  "invalid tss",
  "segment not present",
  "stack segment fault",
  "general protection fault",
  "page fault",
  "reserved",
  "x87 floating point exception",
  "alignment check",
  "machine check",
  "simd floating point exception",
  "virtualization exception",
  "control protection exception",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
  "hypervisor injection exception",
  "vmm communication exception",
  "security exception",
  "reserved",
};

void
interrupt_handler (struct interrupt_frame *frame)
{
  if (frame->vector < 32)
    {
      printf ("%Cexception: %s\nvector: %d error code: %x eip: %x\n", red,
              exception_names[frame->vector], frame->vector, frame->error_code,
              frame->eip);
      panic ("unhandled cpu exception\n");
    }

  uint32_t irq = frame->vector - PIC_MASTER_OFFSET;
  if (irq == 1)
    {
      keyboard_handle_interrupt ();
    }

  // acknowledge interrupt. irqs coming from slave pic need both pics acked
  if (irq >= 8)
    {
      outb (PIC_SLAVE_COMMAND, PIC_EOI);
    }
  outb (PIC_MASTER_COMMAND, PIC_EOI);
}

void
idt_init (uint8_t verbose)
{
  memset (idt, 0, sizeof (idt));
  idtr_descriptor.limit = ((sizeof (struct idt_entry) * TOTAL_INTERRUPTS) - 1);
  idtr_descriptor.base = (uint32_t)&idt[0];
  for (int i = 0; i < TOTAL_INTERRUPTS; i++)
    {
      idt_set (i, no_intr);
    }
  for (int i = 0; i < TOTAL_ISR_STUBS; i++)
    {
      idt_set (i, (void *)isr_stub_table[i]);
    }
  idt_load (&idtr_descriptor);
  if (verbose)
    {
      printf ("idt init");
      init_OK ();
    }
}

void
enable_interrupts (uint8_t verbose)
{
  enable_intr ();
  if (verbose)
    {
      printf ("enable interrupts");
      init_OK ();
    }
}
void
disable_interrupts (void)
{
  disable_intr ();
}

void
idt_set (uint32_t interrupt_num, void *addr)
{
  struct idt_entry *desc = &idt[interrupt_num];
  desc->isr_low = (uint32_t)addr & 0x0000ffff;
  desc->kernel_cs = KERNEL_CODE_SEG;
  desc->zero = 0x00;
  // present, ring 0, 32 bit interrupt gate
  desc->type_attr = 0x8e;
  desc->isr_high = (uint32_t)addr >> 16;
}
