#include "path_parser.h"
#include "../kernel/config.h"
#include "../libc/ctype/ctype.h"
#include "../libc/stdlib/stdlib.h"
#include "../libc/string/string.h"

static int
pathparser_path_valid_format (const char *filename)
{
  int len = strlen (filename);
  return (len >= 3 && isdigit (filename[0])
          && memcmp ((void *)&filename[1], ":/", 2) == 0);
}

static int
pathparser_get_drive_by_path (const char **path)
{
  if (!pathparser_path_valid_format (*path))
    {
      return -INVALID_PATH_ERROR;
    }

  int drive_no = (*path[0]) - 48;

  // add 3 bytes to skip drive number 0:/ 1:/ 2:/
  *path += 3;
  return drive_no;
}

static struct path_root *
pathparser_create_root (int drive_number)
{
  struct path_root *path_r = calloc (sizeof (struct path_root));
  path_r->drive_no = drive_number;
  path_r->first = 0;
  return path_r;
}

static const char *
pathparser_get_path_part (const char **path)
{
  // +1 so a part of MAX_PATH_LEN chars still has a null terminator
  char *result_path_part = calloc (MAX_PATH_LEN + 1);
  int i = 0;
  while (**path != '/' && **path != 0x00)
    {
      result_path_part[i] = **path;
      *path += 1;
      i++;
    }

  if (**path == '/')
    {
      // Skip the forward slash to avoid problems
      *path += 1;
    }

  if (i == 0)
    {
      free (result_path_part);
      result_path_part = 0;
    }

  return result_path_part;
}

struct path_part *
pathparser_parse_path_part (struct path_part *last_part, const char **path)
{
  const char *path_part_str = pathparser_get_path_part (path);
  if (!path_part_str)
    {
      return 0;
    }

  struct path_part *part = calloc (sizeof (struct path_part));
  part->part = path_part_str;
  part->next = 0x00;

  if (last_part)
    {
      last_part->next = part;
    }

  return part;
}

void
pathparser_free (struct path_root *root)
{
  struct path_part *part = root->first;
  while (part)
    {
      struct path_part *next_part = part->next;
      free ((void *)part->part);
      free (part);
      part = next_part;
    }

  free (root);
}

struct path_root *
pathparser_parse (const char *path, const char *current_directory_path)
{
  int res = 0;
  const char *tmp_path = path;
  struct path_root *path_root = 0;

  if (strlen (path) > MAX_PATH_LEN)
    {
      goto out;
    }

  res = pathparser_get_drive_by_path (&tmp_path);
  if (res < 0)
    {
      goto out;
    }

  path_root = pathparser_create_root (res);
  if (!path_root)
    {
      goto out;
    }

  struct path_part *first_part = pathparser_parse_path_part (NULL, &tmp_path);
  if (!first_part)
    {
      goto out;
    }

  path_root->first = first_part;
  struct path_part *part = pathparser_parse_path_part (first_part, &tmp_path);
  while (part)
    {
      part = pathparser_parse_path_part (part, &tmp_path);
    }

out:
  return path_root;
}

int
pathparser_resolve (const char *cwd, const char *path, char *out, int max)
{
  // joined path before normalizing, at most cwd + / + path
  char joined[2 * MAX_PATH_LEN + 2];
  int cwd_len = strlen (cwd);
  int path_len = strlen (path);
  if (cwd_len > MAX_PATH_LEN || path_len > MAX_PATH_LEN || cwd_len < 3)
    {
      return 0;
    }
  if (isdigit (path[0]) && path[1] == ':')
    {
      strcpy (joined, path);
    }
  else if (path[0] == '/')
    {
      // keep the drive of the working directory
      memcpy (joined, cwd, 2);
      strcpy (joined + 2, path);
    }
  else
    {
      strcpy (joined, cwd);
      strcat (joined, "/");
      strcat (joined, path);
    }

  // drive prefix "N:" is kept, then every part is copied unless it is empty,
  // "." or "..", which removes the previous part
  if (max < 4 || joined[2] != '/')
    {
      return 0;
    }
  int len = 2;
  memcpy (out, joined, 2);
  const char *part = joined + 2;
  while (*part)
    {
      while (*part == '/')
        {
          part++;
        }
      const char *end = part;
      while (*end && *end != '/')
        {
          end++;
        }
      int part_len = end - part;
      if (part_len == 1 && part[0] == '.')
        {
          // current directory, nothing to add
        }
      else if (part_len == 2 && part[0] == '.' && part[1] == '.')
        {
          while (len > 2 && out[len - 1] != '/')
            {
              len--;
            }
          if (len > 2)
            {
              len--; // drop the slash too
            }
        }
      else if (part_len > 0)
        {
          if (len + 1 + part_len >= max || len + 1 + part_len > MAX_PATH_LEN)
            {
              return 0;
            }
          out[len++] = '/';
          memcpy (out + len, part, part_len);
          len += part_len;
        }
      part = end;
    }
  if (len == 2)
    {
      out[len++] = '/'; // root directory
    }
  out[len] = '\0';
  return 1;
}
