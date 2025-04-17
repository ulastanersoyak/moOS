#ifndef PATHPARSER_H
#define PATHPARSER_H

struct path_root
{
  int drive_no;
  struct path_part *first;
};

struct path_part
{
  const char *part;
  struct path_part *next;
};

struct path_root *pathparser_parse (const char *path,
                                    const char *current_directory_path);
void pathparser_free (struct path_root *root);

// joins path with the working directory cwd (an absolute path like 0:/dir)
// unless path is absolute ("0:/x" or "/x"), then removes "." and ".." parts.
// writes e.g. "0:/dir/file" to out. returns 0 if the result doesnt fit in max
int pathparser_resolve (const char *cwd, const char *path, char *out, int max);

#endif