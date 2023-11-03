#include "../drivers/screen/terminal.h"
#include "path_parser.h"
#include "../libc/string/string.h"
#include "../libc/ctype/ctype.h"
#include "../kernel/config.h"
#include "../libc/stdlib/stdlib.h"

static int32_t is_file_path_valid(const char *path){
  size_t size = strlen(path);
  //TODO: replace memcmp with strncmp
  if(size > 3 && size < MAX_PATH_LEN && isdigit(path[0]) && memcmp(path, ":/", 2)){return -INVALID_PATH_ERROR;}
  return OK;
}

static void get_body(const char **path, struct path_body *body){
  // TODO: might be a better way of implementing this
  // TODO: maybe without recursion?
  if(**path == 0){
    return;
  }
  struct path_body *path_body = calloc(sizeof(struct path_body));
  char* body_str = calloc(MAX_PATH_LEN);
  uint32_t i = 0;
  while(**path != '/' && **path != 0x00){
    body_str[i] = **path;
    *path += 1;
    i++;
  }
  if (**path == '/'){
    *path += 1;
  }
  body->body_str = body_str;
  body->next = path_body;
  get_body(path, path_body);
}

struct path_root *get_path(const char *path){
  if(is_file_path_valid(path)){
    return 0;
  }
  const char *temp_path = path;
  struct path_root *root = calloc(sizeof(struct path_root));
  root->drive_no = atoi(&temp_path[0]);
  temp_path+=3;
  get_body(&temp_path,root->body);
  return root;
}

void write_path(const struct path_root *root){
  struct path_body *body = root->body;
  while(body->body_str){
    terminal_writestring(body->body_str);
    terminal_writestring("/");
    body = body->next;
  }
}

void free_path_heap(struct path_root *root){
  struct path_body *body = root->body;
  while(body){
    struct path_body *temp = body;
    free(body->body_str);
    free(body);
    body = temp->next;
  }
  free(root);
}
