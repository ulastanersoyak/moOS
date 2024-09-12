#include "file.h"
#include "../drivers/disk/disk.h"
#include "../drivers/screen/terminal.h"
#include "../kernel/config.h"
#include "../libc/stdio/stdio.h"
#include "../libc/stdlib/stdlib.h"
#include "../libc/string/string.h"
#include "fat/fat16.h"

struct file_system *filesystems[MAX_FILE_SYSTEMS];
struct file_descriptor *file_descriptors[MAX_FILE_SYSTEMS];

static struct file_system **
fs_get_free_filesystem ()
{
  int i = 0;
  for (i = 0; i < MAX_FILE_SYSTEMS; i++)
    {
      if (filesystems[i] == 0)
        {
          return &filesystems[i];
        }
    }

  return 0;
}

void
fs_insert_filesystem (struct file_system *filesystem)
{
  struct file_system **fs;
  fs = fs_get_free_filesystem ();
  if (!fs)
    {
      printf ("Problem inserting filesystem");
      while (1)
        {
        }
    }

  *fs = filesystem;
}

static void
fs_static_load ()
{
  fs_insert_filesystem (fat16_init ());
}

void
fs_load ()
{
  memset (filesystems, 0, sizeof (filesystems));
  fs_static_load ();
}

void
fs_init (int verbose)
{
  if (verbose)
    {
      printf ("file system init");
      init_OK ();
    }
  memset (file_descriptors, 0, sizeof (file_descriptors));
  fs_load ();
}

struct file_system *
fs_resolve (struct disk *disk)
{
  struct file_system *fs = 0;
  for (int i = 0; i < MAX_FILE_SYSTEMS; i++)
    {
      if (filesystems[i] != 0 && filesystems[i]->resolve (disk) == 0)
        {
          fs = filesystems[i];
          break;
        }
    }

  return fs;
}
