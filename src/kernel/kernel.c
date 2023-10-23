#include "../drivers/terminal.h"

void kernel_start() {
  terminal_initialize();
  terminal_writestring("hope it works");
}
