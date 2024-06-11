#ifndef DISK_H
#define DISK_H

#include "../../file_system/file.h"

#include <stdint.h>

typedef uint8_t DISK_TYPE;

// physical hard disk
#define REAL_DISK_TYPE 0

struct disk
{
  DISK_TYPE type;
  uint32_t sector_size;
  int32_t id;
  struct file_system *file_system;
  void *fs_private_data;
};

// starting from logical_block_addr, reads total_block amount of blocks into
// the given buffer
int32_t disk_read (int32_t logical_block_addr, uint32_t total_block,
                   void *buffer);

// returns disk handler of given idx. currently only returns
// main master disk.
struct disk *get_disk (uint32_t idx);

// reads the given disk
int32_t disk_read_block (struct disk *disk, int32_t logical_block_addr,
                         uint32_t total_block, void *buffer);

// searches and initializes all disks available. currently only main master
// disk
void init_main_master_disk (uint8_t verbose);

#endif // !DISK_H
