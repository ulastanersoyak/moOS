#include "stdio.h"
#include "../stdlib/stdlib.h"
#include "../../kernel/config.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"

#include <stdarg.h>

void printf(const char *str, ...){
  va_list args;
  va_start(args, str);
  while(*str != '\0'){
    if(*str == '%'){
      str++;
      if(*str == 'd'){
        int32_t arg = va_arg(args, int32_t);
        terminal_writeint(arg);
        str++;
      }else if(*str == 's'){
        char *arg_str= va_arg(args,char *);
        terminal_writestring(arg_str);
        str++;
      }else if(*str == 'p'){
        void *addr = va_arg(args,void *);
        terminal_writeaddr(addr);
        str++;
      }else if(*str == 'C'){
        enum vga_colour col = va_arg(args,enum vga_colour);
        terminal_setcolour(col);
        str++;
      }
    }
    terminal_putchar(*str);
    str++;
  }
}
