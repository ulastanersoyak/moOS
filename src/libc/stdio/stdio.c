#include "stdio.h"
#include "../stdlib/stdlib.h"
#include "../string/string.h"
#include "../../kernel/config.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"
#include "../../drivers/disk/disk.h"
#include "../../file_system/file.h"
#include "../../file_system/path_parser.h"


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

// static enum FILE_MODE get_file_mode(const char* str){
//   enum FILE_MODE mode = INVALID;
//   if(!(strncmp(str,"r",1))){
//     mode = READ;
//   }else if(!(strncmp(str,"w",1))){
//     mode = WRITE;
//   }else if(!(strncmp(str,"a",1))){
//     mode = APPEND;
//   }
//   return mode;
// }

int32_t fopen(const char* file_name, const char* mode){
  struct path_root *root = get_path(file_name);
  if(!root){
    return -INVALID_PATH_ERROR;
  }
  // return an error if file path is root.
  if(!root->body){
    return -INVALID_PATH_ERROR;
  }
  struct disk_t *disk = get_disk(root->drive_no);
  if(!disk){
    return -INVALID_DISK_ERROR;
  }  
  if(!disk->file_system){
   return -IO_ERROR; 
  }
  // enum FILE_MODE file_mode = get_file_mode(mode);

  return OK;
}
