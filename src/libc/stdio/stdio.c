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

extern struct file_descriptor *file_descriptors[MAX_FILE_SYSTEMS];

void
printf (const char *str, ...)
{
  va_list args;
  va_start (args, str);
  while (*str != '\0')
    {
      if (*str == '%')
        {
          str++;
          if (*str == 'd')
            {
              int32_t arg = va_arg (args, int32_t);
              terminal_writeint (arg);
              str++;
            }
          else if (*str == 's')
            {
              char *arg_str = va_arg (args, char *);
              terminal_writestring (arg_str);
              str++;
            }
          else if (*str == 'C')
            {
              enum vga_colour col = va_arg (args, enum vga_colour);
              terminal_setcolour (col);
              str++;
            }
        }
      terminal_putchar (*str);
      str++;
    }
}

FILE_MODE
file_get_mode_by_string (const char *str)
{
  FILE_MODE mode = FILE_MODE_INVALID;
  if (strncmp (str, "r", 1) == 0)
    {
      mode = FILE_MODE_READ;
    }
  else if (strncmp (str, "w", 1) == 0)
    {
      mode = FILE_MODE_WRITE;
    }
  else if (strncmp (str, "a", 1) == 0)
    {
      mode = FILE_MODE_APPEND;
    }
  return mode;
}

static struct file_descriptor *
file_get_descriptor (int fd)
{
  if (fd <= 0 || fd >= MAX_FILE_DESCS)
    {
      return 0;
    }

  // Descriptors start at 1
  int index = fd - 1;
  return file_descriptors[index];
}

static int
file_new_descriptor (struct file_descriptor **desc_out)
{
  int res = -NO_MEMORY_ERROR;
  for (int i = 0; i < MAX_FILE_DESCS; i++)
    {
      if (file_descriptors[i] == 0)
        {
          struct file_descriptor *desc
              = calloc (sizeof (struct file_descriptor));
          // Descriptors start at 1
          desc->index = i + 1;
          file_descriptors[i] = desc;
          *desc_out = desc;
          res = 0;
          break;
        }
    }

  return res;
}

int
fopen (const char *filename, const char *mode_str)
{
  int res = 0;
  struct path_root *root_path = pathparser_parse (filename, NULL);
  if (!root_path)
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }

  // cannot have just a root path 0:/ 0:/test.txt
  if (!root_path->first)
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }

  // ensure the disk we are reading from exists
  struct disk *disk = get_disk (root_path->drive_no);
  if (!disk)
    {
      res = -IO_ERROR;
      goto out;
    }

  if (!disk->file_system)
    {
      res = -IO_ERROR;
      goto out;
    }

  FILE_MODE mode = file_get_mode_by_string (mode_str);
  if (mode == FILE_MODE_INVALID)
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }

  void *descriptor_priv_data
      = disk->file_system->open (disk, root_path->first, mode);
  if (descriptor_priv_data < 0)
    {
      res = ERROR_I (descriptor_priv_data);
      goto out;
    }

  struct file_descriptor *desc = 0;
  res = file_new_descriptor (&desc);
  if (res < 0)
    {
      goto out;
    }
  desc->file_system = disk->file_system;
  desc->priv = descriptor_priv_data;
  desc->disk = disk;
  res = desc->index;

out:
  // fopen shouldnt return negative values
  if (res < 0)
    {
      res = 0;
    }

  return res;
}

int
fstat (int fd, struct file_stat *stat)
{
  int res = 0;
  struct file_descriptor *desc = file_get_descriptor (fd);
  if (!desc)
    {
      res = -IO_ERROR;
      goto out;
    }

  res = desc->file_system->stat (desc->disk, desc->priv, stat);
out:
  return res;
}

static void
file_free_descriptor (struct file_descriptor *desc)
{
  file_descriptors[desc->index - 1] = 0x00;
  free (desc);
}

int
fclose (int fd)
{
  int res = 0;
  struct file_descriptor *desc = file_get_descriptor (fd);
  if (!desc)
    {
      res = -IO_ERROR;
      goto out;
    }

  res = desc->file_system->close (desc->priv);
  if (res == OK)
    {
      file_free_descriptor (desc);
    }
out:
  return res;
}

int
fseek (int fd, int offset, FILE_SEEK_MODE whence)
{
  int res = 0;
  struct file_descriptor *desc = file_get_descriptor (fd);
  if (!desc)
    {
      res = -IO_ERROR;
      goto out;
    }

  res = desc->file_system->seek (desc->priv, offset, whence);
out:
  return res;
}
int
fread (void *ptr, uint32_t size, uint32_t nmemb, int fd)
{
  int res = 0;
  if (size == 0 || nmemb == 0 || fd < 1)
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }

  struct file_descriptor *desc = file_get_descriptor (fd);
  if (!desc)
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }

  res = desc->file_system->read (desc->disk, desc->priv, size, nmemb,
                                 (char *)ptr);
out:
  return res;
}
