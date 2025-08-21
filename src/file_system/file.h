#ifndef FILE_H
#define FILE_H

#include "path_parser.h"
#include <stdint.h>

typedef unsigned int FILE_SEEK_MODE;
enum
{
  SEEK_SET,
  SEEK_CUR,
  SEEK_END
};

typedef unsigned int FILE_MODE;
enum
{
  FILE_MODE_READ,
  FILE_MODE_WRITE,
  FILE_MODE_APPEND,
  FILE_MODE_INVALID
};

enum
{
  FILE_STAT_READ_ONLY = 0b00000001
};

typedef unsigned int FILE_STAT_FLAGS;

struct disk;
typedef void *(*FS_OPEN_FUNCTION) (struct disk *disk, struct path_part *path,
                                   FILE_MODE mode);
typedef int (*FS_READ_FUNCTION) (struct disk *disk, void *priv, uint32_t size,
                                 uint32_t nmemb, char *out);
typedef int (*FS_WRITE_FUNCTION) (struct disk *disk, void *priv, uint32_t size,
                                  uint32_t nmemb, const char *in);
typedef int (*FS_RESOLVE_FUNCTION) (struct disk *disk);

typedef int (*FS_CLOSE_FUNCTION) (void *priv);

typedef int (*FS_SEEK_FUNCTION) (void *priv, int32_t offset,
                                 FILE_SEEK_MODE seek_mode);

// long (vfat) names up to 63 chars plus the null terminator. must match
// struct dirent in src/user/lib/moos.h
#define MAX_FILENAME_LEN 64

struct file_dirent
{
  char name[MAX_FILENAME_LEN];
  uint32_t size;
  uint8_t is_dir;
  uint16_t date; // last modification, fat format (see rtc_fat_date)
  uint16_t time;
};

// fills out with up to max entries of the directory at path. path is 0 for
// the root directory. returns number of entries or a negative error
typedef int (*FS_LISTDIR_FUNCTION) (struct disk *disk, struct path_part *path,
                                    struct file_dirent *out, int max);

// removes a file or an empty directory
typedef int (*FS_REMOVE_FUNCTION) (struct disk *disk, struct path_part *path);

typedef int (*FS_MKDIR_FUNCTION) (struct disk *disk, struct path_part *path);

struct file_stat
{
  FILE_STAT_FLAGS flags;
  uint32_t filesize;
};

typedef int (*FS_STAT_FUNCTION) (struct disk *disk, void *priv,
                                 struct file_stat *stat);

struct file_system
{
  // file_system should return zero from resolve if the provided disk is using
  // its file_system
  FS_RESOLVE_FUNCTION resolve;
  FS_OPEN_FUNCTION open;
  FS_READ_FUNCTION read;
  FS_SEEK_FUNCTION seek;
  FS_STAT_FUNCTION stat;
  FS_CLOSE_FUNCTION close;
  FS_LISTDIR_FUNCTION listdir;
  FS_WRITE_FUNCTION write;
  FS_REMOVE_FUNCTION remove;
  FS_MKDIR_FUNCTION mkdir;
  char name[20];
};

struct file_descriptor
{
  // The descriptor index
  int index;
  struct file_system *file_system;

  // Private data for internal file descriptor
  void *priv;

  // The disk that the file descriptor should be used on
  struct disk *disk;
};

void fs_init (int verbose);

void fs_insert_filesystem (struct file_system *filesystem);
struct file_system *fs_resolve (struct disk *disk);
#endif
