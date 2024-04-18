#ifndef KPANIC_H
#define KPANIC_H

#include "../drivers/screen/vga.h"
#include "../libc/stdio/stdio.h"

void panic(const char *str) {
  printf("%C kernel panic:\n%s", red, str);
  for (;;) {
  }
}

#endif // !KPANIC_H
