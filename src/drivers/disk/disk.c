#include "disk.h"
#include "../../kernel/config.h"
#include "../../kernel/io/io.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"
#include "../screen/terminal.h"

#include <stddef.h>

struct disk main_master_disk;
// TODO: ADD ALL DISK TYPES
int32_t
disk_read (int32_t logical_block_addr, uint32_t total_block, void *buffer)
{
  // works same as bootloader disk read
  outb (0x1F6, (logical_block_addr >> 24) | 0xE0);
  outb (0x1F2, total_block);
  outb (0x1F3, (uint8_t)(logical_block_addr & 0xFF));
  outb (0x1F4, (uint8_t)(logical_block_addr >> 8));
  outb (0x1F5, (uint8_t)(logical_block_addr >> 16));
  outb (0x1F7, 0x20);
  uint16_t *buff = (uint16_t *)buffer;
  for (size_t i = 0; i < total_block; i++)
    {
      uint8_t c = insb (0x1F7);
      // wait until drive is not busy (0x80) and has data ready (0x08)
      while ((c & 0x80) || !(c & 0x08))
        {
          c = insb (0x1F7);
        }
      // read 256 times since insw reads a word which is 2 byte.
      // 256x2 = 512 byte (1 sector)
      for (size_t j = 0; j < 256; j++)
        {
          *buff = insw (0x1F0);
          buff++;
        }
    }
  return OK;
}

static void
disk_wait_ready (void)
{
  // wait until drive is not busy (0x80) and ready for data (0x08)
  uint8_t c = insb (0x1F7);
  while ((c & 0x80) || !(c & 0x08))
    {
      c = insb (0x1F7);
    }
}

int32_t
disk_write (int32_t logical_block_addr, uint32_t total_block,
            const void *buffer)
{
  outb (0x1F6, (logical_block_addr >> 24) | 0xE0);
  outb (0x1F2, total_block);
  outb (0x1F3, (uint8_t)(logical_block_addr & 0xFF));
  outb (0x1F4, (uint8_t)(logical_block_addr >> 8));
  outb (0x1F5, (uint8_t)(logical_block_addr >> 16));
  outb (0x1F7, 0x30); // write sectors
  const uint16_t *buff = (const uint16_t *)buffer;
  for (size_t i = 0; i < total_block; i++)
    {
      disk_wait_ready ();
      for (size_t j = 0; j < 256; j++)
        {
          outw (0x1F0, *buff);
          buff++;
        }
    }
  // make sure the data reached the disk, not just the drive cache
  outb (0x1F7, 0xE7);
  while (insb (0x1F7) & 0x80)
    {
    }
  return OK;
}

// just works on disk 0 (main mastar disk)
//  TODO: maybe add more disks
struct disk *
get_disk (uint32_t idx)
{
  if (idx != 0)
    {
      return 0;
    }
  return &main_master_disk;
}

struct mbr_partition
{
  uint8_t status; // 0x80 bootable, 0x00 inactive
  uint8_t chs_first[3];
  uint8_t type;
  uint8_t chs_last[3];
  uint32_t lba_first;
  uint32_t sectors;
} __attribute__ ((packed));

#define MBR_PARTITION_TABLE 0x1BE
#define MBR_SIGNATURE 0x1FE

// returns the first sector of the first fat16 partition, or 0 if sector 0 has
// no partition table (the fat16 boot sector of the moOS bootloader layout
// has zeros where the table would be)
static uint32_t
disk_find_fat_partition (void)
{
  uint8_t sector[MASTER_MAIN_DISK_SECTOR_SIZE];
  if (disk_read (0, 1, sector) < 0 || sector[MBR_SIGNATURE] != 0x55
      || sector[MBR_SIGNATURE + 1] != 0xAA)
    {
      return 0;
    }
  struct mbr_partition *table
      = (struct mbr_partition *)(sector + MBR_PARTITION_TABLE);
  for (int i = 0; i < 4; i++)
    {
      uint8_t type = table[i].type;
      // fat16 < 32 mb, fat16 and fat16 lba
      if ((table[i].status == 0x00 || table[i].status == 0x80)
          && (type == 0x04 || type == 0x06 || type == 0x0E)
          && table[i].lba_first > 0)
        {
          return table[i].lba_first;
        }
    }
  return 0;
}

void
init_main_master_disk (uint8_t verbose)
{
  memset (&main_master_disk, 0, sizeof (main_master_disk));
  main_master_disk.type = REAL_DISK_TYPE;
  main_master_disk.sector_size = MASTER_MAIN_DISK_SECTOR_SIZE;
  main_master_disk.fs_private_data = 0;
  main_master_disk.id = 0;
  main_master_disk.lba_offset = disk_find_fat_partition ();
  main_master_disk.file_system = fs_resolve (&main_master_disk);
  if (verbose)
    {
      printf ("master disk driver init");
      if (!main_master_disk.file_system)
        {
          init_ER ();
        }
      else
        {
          printf (" <FS: %s", main_master_disk.file_system->name);
          if (main_master_disk.lba_offset)
            {
              printf (", partition at sector %u", main_master_disk.lba_offset);
            }
          printf (">");
          init_OK ();
        }
    }
}

int32_t
disk_read_block (struct disk *disk, int32_t logical_block_addr,
                 uint32_t total_block, void *buffer)
{
  if (disk != &main_master_disk)
    {
      return -IO_ERROR;
    }
  return disk_read (disk->lba_offset + logical_block_addr, total_block,
                    buffer);
}

int32_t
disk_write_block (struct disk *disk, int32_t logical_block_addr,
                  uint32_t total_block, const void *buffer)
{
  if (disk != &main_master_disk)
    {
      return -IO_ERROR;
    }
  return disk_write (disk->lba_offset + logical_block_addr, total_block,
                     buffer);
}
