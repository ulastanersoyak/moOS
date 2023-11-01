#include "path_parser.h"
#include "../libc/string/string.h"
#include "../libc/ctype/ctype.h"
#include "../kernel/config.h"

int32_t is_file_path_valid(const char *path){
  // path len > 3 && 0:/ 1:/ 2:/ && path[0] is digit 
  if(strlen(path) > 3 && isdigit(path[0]) && memcmp(path, ":/", 2)){return -INVALID_PATH_ERROR;}
  return OK;
}
