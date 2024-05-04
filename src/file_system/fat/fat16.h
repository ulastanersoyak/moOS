#ifndef FAT16_H
#define FAT16_H

#include "../file.h"
#include <stdint.h>

struct file_system *fat16_init (void);

int32_t fat16_resolve (struct disk_t *disk);

void *fat16_open (struct disk_t *disk, struct path_root *root, enum FILE_MODE);

uint32_t fat16_read (struct disk_t *disk, void *desc, uint32_t size,
                     uint32_t nmemb, char *out);

#endif // !FAT16_H
