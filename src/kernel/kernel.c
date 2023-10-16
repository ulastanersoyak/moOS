#include "kernel.h"
#include "../drivers/terminal.c"

void kernel_main(void) {
  terminal_initialize();
  terminal_setcolour(white);
  shrigs();
}
