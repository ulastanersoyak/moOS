#include "../drivers/terminal.h"
#include "../drivers/vga.h"
void kernel_start() {
  terminal_initialize();
  terminal_writestring("hello");
}
