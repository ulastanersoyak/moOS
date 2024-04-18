#include "stdio.h"
#include "../../drivers/disk/disk.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"
#include "../../file_system/file.h"
#include "../../file_system/path_parser.h"
#include "../../kernel/config.h"
#include "../ctype/ctype.h"
#include "../stdlib/stdlib.h"
#include "../string/string.h"

#include <stdarg.h>

void printf(const char *str, ...) {
  va_list args;
  va_start(args, str);
  while (*str != '\0') {
    if (*str == '%') {
      str++;
      if (*str == 'd') {
        int32_t arg = va_arg(args, int32_t);
        terminal_writeint(arg);
        str++;
      } else if (*str == 's') {
        char *arg_str = va_arg(args, char *);
        terminal_writestring(arg_str);
        str++;
      } else if (*str == 'p') {
        void *addr = va_arg(args, void *);
        terminal_writeaddr(addr);
        str++;
      } else if (*str == 'P') {
        void *path = va_arg(args, void *);
        write_path(path);
        str++;
      } else if (*str == 'C') {
        enum vga_colour col = va_arg(args, enum vga_colour);
        terminal_setcolour(col);
        str++;
      }
    }
    terminal_putchar(*str);
    str++;
  }
}

static int32_t strcmp_without_case_sens(const char *str1, const char *str2) {
  if (strlen(str1) != strlen(str2)) {
    return -1;
  }
  for (size_t i = 0; i < strlen(str1); i++) {
    if (tolower(str1[i]) != tolower(str2[i])) {
      return -1;
    }
  }
  return 0;
}

static enum FILE_MODE get_file_mode(const char *str) {
  enum FILE_MODE mode = INVALID;
  if (!(strncmp(str, "r", 1))) {
    mode = READ;
  } else if (!(strncmp(str, "w", 1))) {
    mode = WRITE;
  } else if (!(strncmp(str, "a", 1))) {
    mode = APPEND;
  }
  return mode;
}

int32_t fopen(const char *file_name, const char *file_mode) {
  struct path_root *root = get_path(file_name);
  if (!root) {
    return -INVALID_PATH_ERROR;
  }
  // return an error if file path is root.
  if (!root->body->body_str) {
    return -INVALID_PATH_ERROR;
  }
  // check if disk has a driver
  struct disk_t *disk = get_disk(root->drive_no);
  if (!disk) {
    return -INVALID_DISK_ERROR;
  }
  // check if disk has a file system kernel can handle
  if (!disk->file_system) {
    return -IO_ERROR;
  }
  enum FILE_MODE mode = get_file_mode(file_mode);
  if (mode == INVALID) {
    return -INVALID_ARG_ERROR;
  }
  void *desc_private = disk->file_system->open_fn(disk, root, mode);
  if (IS_ERR(desc_private)) {
    return -ERROR_I(desc_private);
  }
  struct file_desc *desc = 0;
  int32_t rs = file_desc_init(&desc);
  if (rs < 0) {
    return rs;
  }
  desc->fs = disk->file_system;
  desc->priv = desc_private;
  desc->disk = disk;
  rs = desc->idx;
  if (rs < 0) {
    rs = 0;
  }
  return rs;
}
