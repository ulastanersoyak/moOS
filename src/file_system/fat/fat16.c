#include "../file.h"
#include "fat16.h"
#include "../../libc/string/string.h"
#include "../../kernel/config.h"

struct file_system fat16_fs ={
  .resolve_fn = fat16_resolve,
  .open_fn = fat16_open,

};

struct file_system *fat16_init(){
  strcpy(fat16_fs.fs_name, "FAT16");
  return &fat16_fs;
}

int32_t fat16_resolve(struct disk_t *disk){
  
  return 0;
}

void *fat16_open(struct disk_t *disk, struct path_root *root, enum FILE_MODE mode){
  return 0;
}
