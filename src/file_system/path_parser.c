#include "../drivers/screen/terminal.h"

#include "path_parser.h"
#include "../libc/string/string.h"
#include "../libc/ctype/ctype.h"
#include "../kernel/config.h"
#include "../kernel/kmem/kheap.h"
#include "../libc/stdlib/stdlib.h"

static int32_t is_file_path_valid(const char *path){
  // path len > 3 && 0:/ 1:/ 2:/ && path[0] is digit 
  size_t size = strlen(path);
  //TODO: replace memcmp with strncmp
  if(size > 3 && size < MAX_PATH_LEN && isdigit(path[0]) && memcmp(path, ":/", 2)){return -INVALID_PATH_ERROR;}
  return OK;
}

char *get_body_part(const char **path){
  // TODO: might be a better way of implementing this
  char* result_path_part = kcalloc(MAX_PATH_LEN);
  uint32_t i = 0;
  while(**path != '/' && **path != 0x00)
  {
    result_path_part[i] = **path;
    *path += 1;
    i++;
  }
  if (**path == '/')
  {
      *path += 1;
  }
  if(i == 0)
  {
    kfree(result_path_part);
    result_path_part = 0;
  }
  return result_path_part;
}


struct path_root *get_path(const char *path){
  if(is_file_path_valid(path)){
    return 0;
  }
  struct path_root *root = kcalloc(sizeof(struct path_root));
  root->drive_no = atoi(&path[0]);
  root->body = 0;
  path+=3;
  return root;
}
