#ifndef PATH_PARSER_H
#define PATH_PARSER_H

#include <stdint.h>

struct path_root
{
  int32_t drive_no;
  struct path_body *body;
};

struct path_body
{
  char *body_str;
  struct path_body *next;
};

// returns 0 if error
struct path_root *get_path (const char *path);

void free_path_heap (struct path_root *root);

// no real usage. just for debug. TODO: might implement it in a future printf
void write_path (const struct path_root *root);
#endif // !PATH_PARSER_H
