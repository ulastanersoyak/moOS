#include "../drivers/screen.h"

void kernel_start() {
  char *vm = (char *)0xb8000;
  vm[0] = 'a';
  vm[2] = 'b';
  // kprint("\ns");
}
