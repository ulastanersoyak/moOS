#ifndef KPANIC_H
#define KPANIC_H

#include "../drivers/screen/vga.h"
#include "../libc/stdio/stdio.h"
#include "idt/idt.h"

static void
panic (const char *str)
{
  disable_interrupts ();
  printf ("%Ckernel panic:\n%s", red, str);
  for (;;)
    {
    }
}

#endif // !KPANIC_H
