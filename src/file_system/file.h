#ifndef FILE_H
#define FILE_H

#include <stdint.h>

#include "path_parser.h"

enum FILE_SEEK_MODE{
  SEEK_SET,
  SEEK_CUR,
  SEEK_END
};

enum FILE_MODE
{ 
  READ,
  WRITE,
  APPEND,
  INVALID
};

struct disk_t;

// general file system interface that opens a file in given disk and path with given file modes
typedef void*(*FS_OPEN)(struct disk_t *disk, struct path_root *root, enum FILE_MODE mode);

// resolves if file system can operate on the given disk
typedef int32_t(*FS_RESOLVE)(struct disk_t *disk);

struct file_system{
  // file system should return 0 if disk is usable
  FS_RESOLVE resolve_fn;
  FS_OPEN open_fn;  
  char *fs_name;
};

struct file_desc{
  int32_t idx;
  struct file_system *fs;
  // internal file descriptors private data
  void *priv;
  // disk that file desc used on
  struct disk_t *disk;
};

void file_system_init(uint8_t verbose);  

void add_file_system(struct file_system *fs);

struct file_system *fs_resolve(struct disk_t *disk);

#endif// !FILE_H
