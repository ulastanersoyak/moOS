#include "stdio.h"
#include "../stdlib/stdlib.h"
#include "../../kernel/config.h"
#include "../../drivers/screen/terminal.h"

#include <stdarg.h>

// struct printf_args{
//   size_t count;
//   int32_t arg_pos[MAX_STRING_FORMAT_CHAR];
// };

// static struct printf_args *get_arg_count(const char *str){
//   uint32_t arg_count = 0;
//   const char *temp_str = str;
//   int32_t i = 0;
//   int32_t pos = 0;
//   struct printf_args* args = calloc(sizeof(struct printf_args)); 
//   while(*temp_str != 0){
//     if(*temp_str == '%' && *temp_str+1 == '%'){
//       args->arg_pos[pos]= i;
//       pos++;
//     }
//     i++;
//     temp_str+=1;
//   }
//   return args;
// }

void printf(const char *str, ...){
  va_list args;
  va_start(args, str);
  while(*str != '\0'){
    if(*str == '%'){
      str++;
      if(*str == 'd'){
        int32_t arg = va_arg(args, int);
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
      }
    }
    terminal_putchar(*str);
    str++;
  }
}
