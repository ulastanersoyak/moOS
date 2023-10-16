#include "kernel.h"
#include "../drivers/terminal.h"

void kernel_main(void) {
  terminal_initialize();
  cowsay("linked! mf");
}
