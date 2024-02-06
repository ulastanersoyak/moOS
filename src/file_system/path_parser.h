#ifndef PATH_PARSER_H
#define PATH_PARSER_H

#include <stdint.h>

struct path_root{
  int32_t drive_no;
  struct path_body *body;
};

struct path_body{
  char *body;
  struct path_body *next;
};

// returns 0 if error
struct path_root *get_path(const char* path);

#endif // !PATH_PARSER_H
