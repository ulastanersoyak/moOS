#include "kernel.h"
#include "../drivers/terminal.h"
#include "../libc/string/string.h"
#include "idt/idt.h"

void kernel_main(void) {
  terminal_initialize();
  idt_init();
  terminal_writestring("hello kernel!\nlinked correctly yay");
}
