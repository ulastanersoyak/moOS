#include "idt.h"
#include "../../drivers/screen/terminal.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"
#include "../config.h"
#include "../io/io.h"

// idt table array
struct idt_entry idt[TOTAL_INTERRUPTS];
// idt tables descriptor that contains address and length of idt
struct idtr_desc idtr_descriptor;

extern void int21h ();
extern void no_intr ();

extern void
int21h_handler ()
{
  terminal_writestring ("keyboard pressed");
  outb (0x20, 0x20);
}

void
no_intr_handler ()
{
  outb (0x20, 0x20);
}

void
divide_by_zero_exc ()
{
  terminal_writestring ("divide by zero exception occured\n");
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
  idt_set (0, divide_by_zero_exc);
  idt_set (0x21, int21h);
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
disble_interrupts (void)
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
  desc->type_attr = 0xee;
  desc->isr_high = (uint32_t)addr >> 16;
}
