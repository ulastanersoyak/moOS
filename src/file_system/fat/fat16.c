#include "fat16.h"
#include "../../drivers/disk/disk.h"
#include "../../drivers/disk/disk_stream.h"
#include "../../drivers/rtc/rtc.h"
#include "../../kernel/config.h"
#include "../../libc/ctype/ctype.h"
#include "../../libc/stdlib/stdlib.h"
#include "../../libc/string/string.h"
#include <stdint.h>

#define FAT16_SIGNATURE 0x29
#define FAT16_FAT_ENTRY_SIZE 0x02
#define FAT16_BAD_SECTOR 0xFFF7
#define FAT16_UNUSED 0x00
// fat entries from 0xFFF8 up mark the last cluster of a chain
#define FAT16_END_OF_CHAIN 0xFFF8
#define FAT16_RESERVED_START 0xFFF0
#define FAT16_RESERVED_END 0xFFF6

// first byte of a directory item filename
#define FAT_ENTRY_END 0x00
#define FAT_ENTRY_DELETED 0xE5

// attribute value used by vfat long filename entries
#define FAT_FILE_LONG_NAME 0x0F
// the first long name entry of a name (stored first, holds its end) has this
// bit set in its sequence number
#define FAT_LFN_LAST 0x40
#define FAT_LFN_SEQ_MASK 0x1F
#define FAT_LFN_CHARS 13
// longest name we create, the dirent can hold one more char for the null
#define FAT_LFN_MAX (MAX_FILENAME_LEN - 1)
#define FAT_LFN_MAX_ENTRIES                                                   \
  ((FAT_LFN_MAX + FAT_LFN_CHARS - 1) / FAT_LFN_CHARS)

// windows nt keeps the case of 8.3 names in the reserved byte instead of
// writing a long name for "readme.txt"
#define FAT_NAME_LOWER_BASE 0x08
#define FAT_NAME_LOWER_EXT 0x10

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

// Fat directory entry attributes bitmask
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80

struct fat_header_extended
{
  uint8_t drive_number;
  uint8_t win_nt_bit;
  uint8_t signature;
  uint32_t volume_id;
  uint8_t volume_id_string[11];
  uint8_t system_id_string[8];
} __attribute__ ((packed));

struct fat_header
{
  uint8_t short_jmp_ins[3];
  uint8_t oem_identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_copies;
  uint16_t root_dir_entries;
  uint16_t number_of_sectors;
  uint8_t media_type;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t number_of_heads;
  uint32_t hidden_setors;
  uint32_t sectors_big;
} __attribute__ ((packed));

struct fat_h
{
  struct fat_header primary_header;
  union fat_h_e
  {
    struct fat_header_extended extended_header;
  } shared;
};

struct fat_directory_item
{
  uint8_t filename[8];
  uint8_t ext[3];
  uint8_t attribute;
  uint8_t reserved;
  uint8_t creation_time_tenths_of_a_sec;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access;
  uint16_t high_16_bits_first_cluster;
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint16_t low_16_bits_first_cluster;
  uint32_t filesize;
} __attribute__ ((packed));

// vfat long name entry. a long name is stored as a run of these right before
// the 8.3 entry of the file, last part first. each holds 13 ucs-2 chars
struct fat_lfn_entry
{
  uint8_t seq;
  uint16_t name1[5];
  uint8_t attribute; // FAT_FILE_LONG_NAME
  uint8_t type;
  uint8_t checksum; // of the 8.3 name it belongs to
  uint16_t name2[6];
  uint16_t first_cluster; // always 0
  uint16_t name3[2];
} __attribute__ ((packed));

struct fat_directory
{
  struct fat_directory_item *item;
  int total;
  int sector_pos;
  int ending_sector_pos;
  uint32_t first_cluster; // 0 for the root directory
};

struct fat_item
{
  union
  {
    struct fat_directory_item *item;
    struct fat_directory *directory;
  };

  FAT_ITEM_TYPE type;
};

struct fat_file_descriptor
{
  struct fat_item *item;
  uint32_t pos;
  FILE_MODE mode;
  // where the directory item of the file is stored on disk, written back
  // after every write so size and first cluster stay up to date
  uint32_t dir_entry_pos;
};

struct fat_private
{
  struct fat_h header;
  struct fat_directory root_directory;

  // Used to stream data clusters
  struct disk_stream *cluster_read_stream;
  // Used to stream the file allocation table
  struct disk_stream *fat_read_stream;

  // Used in situations where we stream the directory
  struct disk_stream *directory_stream;

  // where the search for a free cluster starts
  uint32_t next_free_cluster;
};

int fat16_resolve (struct disk *disk);
void *fat16_open (struct disk *disk, struct path_part *path, FILE_MODE mode);
int fat16_read (struct disk *disk, void *descriptor, uint32_t size,
                uint32_t nmemb, char *out_ptr);
int fat16_seek (void *private, int32_t offset, FILE_SEEK_MODE seek_mode);
int fat16_stat (struct disk *disk, void *private, struct file_stat *stat);
int fat16_close (void *private);
int fat16_listdir (struct disk *disk, struct path_part *path,
                   struct file_dirent *out, int max);
int fat16_write (struct disk *disk, void *descriptor, uint32_t size,
                 uint32_t nmemb, const char *in);
int fat16_remove (struct disk *disk, struct path_part *path);
int fat16_mkdir (struct disk *disk, struct path_part *path);

struct file_system fat16_fs = { .resolve = fat16_resolve,
                                .open = fat16_open,
                                .read = fat16_read,
                                .seek = fat16_seek,
                                .stat = fat16_stat,
                                .close = fat16_close,
                                .listdir = fat16_listdir,
                                .write = fat16_write,
                                .remove = fat16_remove,
                                .mkdir = fat16_mkdir };

struct file_system *
fat16_init ()
{
  strcpy (fat16_fs.name, "FAT16");
  return &fat16_fs;
}

static void
fat16_init_private (struct disk *disk, struct fat_private *fat_private)
{
  memset (fat_private, 0, sizeof (struct fat_private));
  fat_private->cluster_read_stream = get_disk_stream (disk->id);
  fat_private->fat_read_stream = get_disk_stream (disk->id);
  fat_private->directory_stream = get_disk_stream (disk->id);
}

int
fat16_sector_to_absolute (struct disk *disk, int sector)
{
  return sector * disk->sector_size;
}

// counts directory items up to the end marker. deleted items are counted too
// since the items are later read as one raw array and indexed directly.
int
fat16_get_total_items_for_directory (struct disk *disk,
                                     uint32_t directory_start_sector,
                                     int max_items)
{
  struct fat_directory_item item;
  struct fat_private *fat_private = disk->fs_private_data;

  int i = 0;
  int directory_start_pos = directory_start_sector * disk->sector_size;
  struct disk_stream *stream = fat_private->directory_stream;
  stream_seek (stream, directory_start_pos);

  for (i = 0; i < max_items; i++)
    {
      if (disk_stream_read (stream, &item, sizeof (item)) != OK)
        {
          return -IO_ERROR;
        }

      if (item.filename[0] == FAT_ENTRY_END)
        {
          break;
        }
    }

  return i;
}

int
fat16_get_root_directory (struct disk *disk, struct fat_private *fat_private,
                          struct fat_directory *directory)
{
  int res = 0;
  struct fat_directory_item *dir = 0x00;
  struct fat_header *primary_header = &fat_private->header.primary_header;
  int root_dir_sector_pos
      = (primary_header->fat_copies * primary_header->sectors_per_fat)
        + primary_header->reserved_sectors;
  int root_dir_entries = fat_private->header.primary_header.root_dir_entries;
  int root_dir_size = (root_dir_entries * sizeof (struct fat_directory_item));
  int total_sectors = root_dir_size / disk->sector_size;
  if (root_dir_size % disk->sector_size)
    {
      total_sectors += 1;
    }

  int total_items = fat16_get_total_items_for_directory (
      disk, root_dir_sector_pos, root_dir_entries);
  if (total_items < 0)
    {
      res = total_items;
      goto err_out;
    }

  dir = calloc (root_dir_size);
  if (!dir)
    {
      res = -NO_MEMORY_ERROR;
      goto err_out;
    }

  struct disk_stream *stream = fat_private->directory_stream;
  stream_seek (stream, fat16_sector_to_absolute (disk, root_dir_sector_pos));

  if (disk_stream_read (stream, dir, root_dir_size) != OK)
    {
      res = -IO_ERROR;
      goto err_out;
    }

  directory->item = dir;
  directory->total = total_items;
  directory->sector_pos = root_dir_sector_pos;
  directory->ending_sector_pos = root_dir_sector_pos + total_sectors;
  return res;

err_out:
  if (dir)
    {
      free (dir);
    }

  return res;
}
static void
fat16_free_private (struct fat_private *private)
{
  if (private->root_directory.item)
    {
      free (private->root_directory.item);
    }
  stream_free (private->cluster_read_stream);
  stream_free (private->fat_read_stream);
  stream_free (private->directory_stream);
  free (private);
}

int
fat16_resolve (struct disk *disk)
{
  int res = 0;
  struct fat_private *fat_private = calloc (sizeof (struct fat_private));
  if (!fat_private)
    {
      return -NO_MEMORY_ERROR;
    }
  fat16_init_private (disk, fat_private);

  // helpers below read the private data through the disk
  disk->fs_private_data = fat_private;

  struct disk_stream *stream = get_disk_stream (disk->id);
  if (!stream)
    {
      res = -NO_MEMORY_ERROR;
      goto out;
    }

  if (disk_stream_read (stream, &fat_private->header,
                        sizeof (fat_private->header))
      != OK)
    {
      res = -IO_ERROR;
      goto out;
    }

  if (fat_private->header.shared.extended_header.signature != FAT16_SIGNATURE)
    {
      res = -SIGNATURE_ERROR;
      goto out;
    }

  if (fat16_get_root_directory (disk, fat_private,
                                &fat_private->root_directory)
      != OK)
    {
      res = -IO_ERROR;
      goto out;
    }

out:
  if (stream)
    {
      stream_free (stream);
    }

  if (res < 0)
    {
      fat16_free_private (fat_private);
      disk->fs_private_data = 0;
    }
  return res;
}

void
fat16_to_proper_string (char **out, const char *in, size_t size)
{
  int i = 0;
  while (*in != 0x00 && *in != 0x20)
    {
      **out = *in;
      *out += 1;
      in += 1;
      // We cant process anymore since we have exceeded the input buffer size
      if (i >= size - 1)
        {
          break;
        }
      i++;
    }

  **out = 0x00;
}

void
fat16_get_full_relative_filename (struct fat_directory_item *item, char *out,
                                  int max_len)
{
  memset (out, 0x00, max_len);
  char *out_tmp = out;
  fat16_to_proper_string (&out_tmp, (const char *)item->filename,
                          sizeof (item->filename));
  if (item->ext[0] != 0x00 && item->ext[0] != 0x20)
    {
      *out_tmp++ = '.';
      fat16_to_proper_string (&out_tmp, (const char *)item->ext,
                              sizeof (item->ext));
    }
}

// 8.3 name with the case bits of the reserved byte applied, "readme.txt"
// instead of "README.TXT"
static void
fat16_get_short_name (struct fat_directory_item *item, char *out, int max_len)
{
  fat16_get_full_relative_filename (item, out, max_len);
  uint8_t lower = item->reserved & FAT_NAME_LOWER_BASE;
  for (char *c = out; *c; c++)
    {
      if (*c == '.')
        {
          lower = item->reserved & FAT_NAME_LOWER_EXT;
        }
      else if (lower)
        {
          *c = tolower (*c);
        }
    }
}

static uint8_t
fat16_short_name_checksum (const uint8_t *short_name)
{
  uint8_t sum = 0;
  for (int i = 0; i < 11; i++)
    {
      sum = ((sum & 1) << 7) + (sum >> 1) + short_name[i];
    }
  return sum;
}

static uint16_t *
fat16_lfn_char (struct fat_lfn_entry *entry, int i)
{
  if (i < 5)
    {
      return &entry->name1[i];
    }
  return i < 11 ? &entry->name2[i - 5] : &entry->name3[i - 11];
}

// number of long name entries in front of the 8.3 item idx of directory
// that belong to it, 0 if it has no (valid) long name
static int
fat16_lfn_count (struct fat_directory *directory, int idx)
{
  uint8_t checksum
      = fat16_short_name_checksum (directory->item[idx].filename);
  for (int n = 1; n <= idx && n <= FAT_LFN_SEQ_MASK; n++)
    {
      struct fat_lfn_entry *entry
          = (struct fat_lfn_entry *)&directory->item[idx - n];
      if (entry->attribute != FAT_FILE_LONG_NAME
          || entry->seq == FAT_ENTRY_DELETED
          || (entry->seq & FAT_LFN_SEQ_MASK) != n
          || entry->checksum != checksum)
        {
          return 0;
        }
      if (entry->seq & FAT_LFN_LAST)
        {
          return n;
        }
    }
  return 0;
}

// display name of item idx: its long name if it has one, else the 8.3 name.
// chars outside ascii become '?', names longer than max_len are cut
static void
fat16_get_item_name (struct fat_directory *directory, int idx, char *out,
                     int max_len)
{
  int count = fat16_lfn_count (directory, idx);
  if (count == 0)
    {
      fat16_get_short_name (&directory->item[idx], out, max_len);
      return;
    }
  int len = 0;
  for (int n = 1; n <= count; n++)
    {
      struct fat_lfn_entry *entry
          = (struct fat_lfn_entry *)&directory->item[idx - n];
      for (int i = 0; i < FAT_LFN_CHARS; i++)
        {
          uint16_t c = *fat16_lfn_char (entry, i);
          if (c == 0x0000 || c == 0xFFFF || len == max_len - 1)
            {
              goto done;
            }
          out[len++] = c < 0x80 ? c : '?';
        }
    }
done:
  out[len] = '\0';
}

struct fat_directory_item *
fat16_clone_directory_item (struct fat_directory_item *item, int size)
{
  struct fat_directory_item *item_copy = 0;
  if (size < sizeof (struct fat_directory_item))
    {
      return 0;
    }

  item_copy = calloc (size);
  if (!item_copy)
    {
      return 0;
    }

  memcpy (item_copy, item, size);
  return item_copy;
}

static uint32_t
fat16_get_first_cluster (struct fat_directory_item *item)
{
  return ((uint32_t)item->high_16_bits_first_cluster << 16)
         | item->low_16_bits_first_cluster;
};

static int
fat16_cluster_to_sector (struct fat_private *private, int cluster)
{
  return private->root_directory.ending_sector_pos
         + ((cluster - 2)
            * private->header.primary_header.sectors_per_cluster);
}

static uint32_t
fat16_get_first_fat_sector (struct fat_private *private)
{
  return private->header.primary_header.reserved_sectors;
}

static int
fat16_get_fat_entry (struct disk *disk, int cluster)
{
  int res = -1;
  struct fat_private *private = disk->fs_private_data;
  struct disk_stream *stream = private->fat_read_stream;
  if (!stream)
    {
      goto out;
    }

  uint32_t fat_table_position
      = fat16_get_first_fat_sector (private) * disk->sector_size;
  stream_seek (stream, fat_table_position + (cluster * FAT16_FAT_ENTRY_SIZE));

  uint16_t result = 0;
  res = disk_stream_read (stream, &result, sizeof (result));
  if (res < 0)
    {
      goto out;
    }

  res = result;
out:
  return res;
}
/**
 * Gets the correct cluster to use based on the starting cluster and the offset
 */
static int
fat16_get_cluster_for_offset (struct disk *disk, int starting_cluster,
                              int offset)
{
  int res = 0;
  struct fat_private *private = disk->fs_private_data;
  int size_of_cluster_bytes
      = private->header.primary_header.sectors_per_cluster * disk->sector_size;
  int cluster_to_use = starting_cluster;
  int clusters_ahead = offset / size_of_cluster_bytes;
  for (int i = 0; i < clusters_ahead; i++)
    {
      int entry = fat16_get_fat_entry (disk, cluster_to_use);
      if (entry < 0)
        {
          res = entry;
          goto out;
        }

      if (entry >= FAT16_END_OF_CHAIN)
        {
          // We are at the last entry in the file
          res = -IO_ERROR;
          goto out;
        }

      // Sector is marked as bad?
      if (entry == FAT16_BAD_SECTOR)
        {
          res = -IO_ERROR;
          goto out;
        }

      // Reserved sector?
      if (entry >= FAT16_RESERVED_START && entry <= FAT16_RESERVED_END)
        {
          res = -IO_ERROR;
          goto out;
        }

      if (entry == FAT16_UNUSED)
        {
          res = -IO_ERROR;
          goto out;
        }

      cluster_to_use = entry;
    }

  res = cluster_to_use;
out:
  return res;
}
static int
fat16_read_internal_from_stream (struct disk *disk, struct disk_stream *stream,
                                 int cluster, int offset, int total, void *out)
{
  int res = 0;
  struct fat_private *private = disk->fs_private_data;
  int size_of_cluster_bytes
      = private->header.primary_header.sectors_per_cluster * disk->sector_size;
  int cluster_to_use = fat16_get_cluster_for_offset (disk, cluster, offset);
  if (cluster_to_use < 0)
    {
      res = cluster_to_use;
      goto out;
    }

  int offset_from_cluster = offset % size_of_cluster_bytes;

  int starting_sector = fat16_cluster_to_sector (private, cluster_to_use);
  int starting_pos
      = (starting_sector * disk->sector_size) + offset_from_cluster;
  // clusters of a file arent necessarily adjacent so never read past the
  // end of the current cluster
  int total_to_read = size_of_cluster_bytes - offset_from_cluster;
  if (total_to_read > total)
    {
      total_to_read = total;
    }
  stream_seek (stream, starting_pos);
  res = disk_stream_read (stream, out, total_to_read);
  if (res != OK)
    {
      goto out;
    }

  total -= total_to_read;
  if (total > 0)
    {
      // We still have more to read
      res = fat16_read_internal_from_stream (disk, stream, cluster,
                                             offset + total_to_read, total,
                                             out + total_to_read);
    }

out:
  return res;
}

static int
fat16_read_internal (struct disk *disk, int starting_cluster, int offset,
                     int total, void *out)
{
  struct fat_private *fs_private = disk->fs_private_data;
  struct disk_stream *stream = fs_private->cluster_read_stream;
  return fat16_read_internal_from_stream (disk, stream, starting_cluster,
                                          offset, total, out);
}

void
fat16_free_directory (struct fat_directory *directory)
{
  if (!directory)
    {
      return;
    }

  if (directory->item)
    {
      free (directory->item);
    }

  free (directory);
}

void
fat16_fat_item_free (struct fat_item *item)
{
  if (item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
      fat16_free_directory (item->directory);
    }
  else if (item->type == FAT_ITEM_TYPE_FILE)
    {
      free (item->item);
    }
  free (item);
}

// counts items of a subdirectory by following its cluster chain
static int
fat16_get_total_items_for_subdirectory (struct disk *disk, int cluster)
{
  struct fat_directory_item item;
  int i = 0;
  while (1)
    {
      int res = fat16_read_internal (disk, cluster, i * sizeof (item),
                                     sizeof (item), &item);
      // running off the end of the cluster chain also ends the directory
      if (res < 0 || item.filename[0] == FAT_ENTRY_END)
        {
          break;
        }
      i++;
    }
  return i;
}

struct fat_directory *
fat16_load_fat_directory (struct disk *disk, struct fat_directory_item *item)
{
  int res = 0;
  struct fat_directory *directory = 0;
  struct fat_private *fat_private = disk->fs_private_data;
  if (!(item->attribute & FAT_FILE_SUBDIRECTORY))
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }

  directory = calloc (sizeof (struct fat_directory));
  if (!directory)
    {
      res = -NO_MEMORY_ERROR;
      goto out;
    }

  int cluster = fat16_get_first_cluster (item);
  int cluster_sector = fat16_cluster_to_sector (fat_private, cluster);
  directory->total = fat16_get_total_items_for_subdirectory (disk, cluster);
  directory->sector_pos = cluster_sector;
  directory->first_cluster = cluster;
  if (directory->total == 0)
    {
      goto out;
    }
  int directory_size = directory->total * sizeof (struct fat_directory_item);
  directory->item = calloc (directory_size);
  if (!directory->item)
    {
      res = -NO_MEMORY_ERROR;
      goto out;
    }

  res = fat16_read_internal (disk, cluster, 0x00, directory_size,
                             directory->item);

out:
  if (res != OK)
    {
      fat16_free_directory (directory);
      directory = 0;
    }
  return directory;
}

struct fat_item *
fat16_new_fat_item_for_directory_item (struct disk *disk,
                                       struct fat_directory_item *item)
{
  struct fat_item *f_item = calloc (sizeof (struct fat_item));
  if (!f_item)
    {
      return 0;
    }

  if (item->attribute & FAT_FILE_SUBDIRECTORY)
    {
      f_item->directory = fat16_load_fat_directory (disk, item);
      if (!f_item->directory)
        {
          free (f_item);
          return 0;
        }
      f_item->type = FAT_ITEM_TYPE_DIRECTORY;
      return f_item;
    }

  f_item->type = FAT_ITEM_TYPE_FILE;
  f_item->item
      = fat16_clone_directory_item (item, sizeof (struct fat_directory_item));
  return f_item;
}

// returns the index of the 8.3 item called name in directory or -1. name
// can be the long name or the 8.3 alias ("long file name.txt" or
// "LONGFI~1.TXT"), case is ignored like on windows
static int
fat16_find_index (struct fat_directory *directory, const char *name)
{
  char tmp_filename[MAX_PATH_LEN + 1];
  for (int i = 0; i < directory->total; i++)
    {
      struct fat_directory_item *item = &directory->item[i];
      if (item->filename[0] == FAT_ENTRY_DELETED
          || item->attribute == FAT_FILE_LONG_NAME
          || (item->attribute & FAT_FILE_VOLUME_LABEL))
        {
          continue;
        }
      fat16_get_item_name (directory, i, tmp_filename, sizeof (tmp_filename));
      if (istrncmp (tmp_filename, name, sizeof (tmp_filename)) == 0)
        {
          return i;
        }
      fat16_get_full_relative_filename (item, tmp_filename,
                                        sizeof (tmp_filename));
      if (istrncmp (tmp_filename, name, sizeof (tmp_filename)) == 0)
        {
          return i;
        }
    }
  return -1;
}

struct fat_item *
fat16_find_item_in_directory (struct disk *disk,
                              struct fat_directory *directory,
                              const char *name)
{
  int i = fat16_find_index (directory, name);
  if (i < 0)
    {
      return 0;
    }
  return fat16_new_fat_item_for_directory_item (disk, &directory->item[i]);
}
struct fat_item *
fat16_get_directory_entry (struct disk *disk, struct path_part *path)
{
  struct fat_private *fat_private = disk->fs_private_data;
  struct fat_item *current_item = 0;
  struct fat_item *root_item = fat16_find_item_in_directory (
      disk, &fat_private->root_directory, path->part);
  if (!root_item)
    {
      goto out;
    }

  struct path_part *next_part = path->next;
  current_item = root_item;
  while (next_part != 0)
    {
      if (current_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
          current_item = 0;
          break;
        }

      struct fat_item *tmp_item = fat16_find_item_in_directory (
          disk, current_item->directory, next_part->part);
      fat16_fat_item_free (current_item);
      current_item = tmp_item;
      next_part = next_part->next;
    }
out:
  return current_item;
}

static void *fat16_open_for_write (struct disk *disk, struct path_part *path,
                                   FILE_MODE mode);

void *
fat16_open (struct disk *disk, struct path_part *path, FILE_MODE mode)
{
  struct fat_file_descriptor *descriptor = 0;
  int err_code = 0;
  if (mode != FILE_MODE_READ)
    {
      return fat16_open_for_write (disk, path, mode);
    }

  descriptor = calloc (sizeof (struct fat_file_descriptor));
  if (!descriptor)
    {
      err_code = -NO_MEMORY_ERROR;
      goto err_out;
    }

  descriptor->item = fat16_get_directory_entry (disk, path);
  if (!descriptor->item)
    {
      err_code = -IO_ERROR;
      goto err_out;
    }

  descriptor->pos = 0;
  return descriptor;

err_out:
  if (descriptor)
    {
      free (descriptor);
    }

  return ERROR (err_code);
}

static void
fat16_free_file_descriptor (struct fat_file_descriptor *desc)
{
  fat16_fat_item_free (desc->item);
  free (desc);
}

int
fat16_close (void *private)
{
  fat16_free_file_descriptor ((struct fat_file_descriptor *)private);
  return 0;
}

int
fat16_stat (struct disk *disk, void *private, struct file_stat *stat)
{
  int res = 0;
  struct fat_file_descriptor *descriptor
      = (struct fat_file_descriptor *)private;
  struct fat_item *desc_item = descriptor->item;
  if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
      res = -INVALID_ARG_ERROR;
      goto out;
    }

  struct fat_directory_item *ritem = desc_item->item;
  stat->filesize = ritem->filesize;
  stat->flags = 0x00;

  if (ritem->attribute & FAT_FILE_READ_ONLY)
    {
      stat->flags |= FILE_STAT_READ_ONLY;
    }
out:
  return res;
}

int
fat16_read (struct disk *disk, void *descriptor, uint32_t size, uint32_t nmemb,
            char *out_ptr)
{
  struct fat_file_descriptor *fat_desc = descriptor;
  if (fat_desc->item->type != FAT_ITEM_TYPE_FILE)
    {
      return -INVALID_ARG_ERROR;
    }
  struct fat_directory_item *item = fat_desc->item->item;
  // only read whole members that fit before the end of the file
  uint32_t members = (item->filesize - fat_desc->pos) / size;
  if (members > nmemb)
    {
      members = nmemb;
    }
  if (members == 0)
    {
      return 0;
    }
  int res = fat16_read_internal (disk, fat16_get_first_cluster (item),
                                 fat_desc->pos, members * size, out_ptr);
  if (res < 0)
    {
      return res;
    }
  fat_desc->pos += members * size;
  return members;
}

int
fat16_seek (void *private, int32_t offset, FILE_SEEK_MODE seek_mode)
{
  struct fat_file_descriptor *desc = private;
  struct fat_item *desc_item = desc->item;
  if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
      return -INVALID_ARG_ERROR;
    }

  struct fat_directory_item *ritem = desc_item->item;
  int32_t new_pos = 0;
  switch (seek_mode)
    {
    case SEEK_SET:
      new_pos = offset;
      break;

    case SEEK_END:
      new_pos = (int32_t)ritem->filesize + offset;
      break;

    case SEEK_CUR:
      new_pos = (int32_t)desc->pos + offset;
      break;

    default:
      return -INVALID_ARG_ERROR;
    }

  // seeking to the end of the file is allowed, past it is not
  if (new_pos < 0 || (uint32_t)new_pos > ritem->filesize)
    {
      return -IO_ERROR;
    }
  desc->pos = new_pos;
  return OK;
}

int
fat16_listdir (struct disk *disk, struct path_part *path,
               struct file_dirent *out, int max)
{
  struct fat_private *fat_private = disk->fs_private_data;
  struct fat_directory *directory = &fat_private->root_directory;
  struct fat_item *dir_item = 0;
  if (path)
    {
      dir_item = fat16_get_directory_entry (disk, path);
      if (!dir_item)
        {
          return -NOT_FOUND_ERROR;
        }
      if (dir_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
          fat16_fat_item_free (dir_item);
          return -NOT_DIRECTORY_ERROR;
        }
      directory = dir_item->directory;
    }

  int total = 0;
  for (int i = 0; i < directory->total && total < max; i++)
    {
      struct fat_directory_item *item = &directory->item[i];
      if (item->filename[0] == FAT_ENTRY_DELETED
          || item->attribute == FAT_FILE_LONG_NAME
          || (item->attribute & FAT_FILE_VOLUME_LABEL)
          || item->filename[0] == '.')
        {
          continue;
        }
      fat16_get_item_name (directory, i, out[total].name,
                           sizeof (out[total].name));
      out[total].size = item->filesize;
      out[total].is_dir = (item->attribute & FAT_FILE_SUBDIRECTORY) != 0;
      out[total].date = item->last_mod_date;
      out[total].time = item->last_mod_time;
      total++;
    }

  if (dir_item)
    {
      fat16_fat_item_free (dir_item);
    }
  return total;
}

// --- write support ---

static uint32_t
fat16_cluster_bytes (struct disk *disk)
{
  struct fat_private *private = disk->fs_private_data;
  return private->header.primary_header.sectors_per_cluster
         * disk->sector_size;
}

// number of the last data cluster + 1
static uint32_t
fat16_cluster_limit (struct disk *disk)
{
  struct fat_private *private = disk->fs_private_data;
  struct fat_header *header = &private->header.primary_header;
  uint32_t sectors = header->number_of_sectors ? header->number_of_sectors
                                               : header->sectors_big;
  uint32_t data_start = private->root_directory.ending_sector_pos;
  return (sectors - data_start) / header->sectors_per_cluster + 2;
}

// writes the entry of cluster to every copy of the fat
static int
fat16_set_fat_entry (struct disk *disk, uint32_t cluster, uint16_t value)
{
  struct fat_private *private = disk->fs_private_data;
  struct fat_header *header = &private->header.primary_header;
  for (uint32_t copy = 0; copy < header->fat_copies; copy++)
    {
      uint32_t fat_sector
          = header->reserved_sectors + copy * header->sectors_per_fat;
      stream_seek (private->fat_read_stream,
                   fat_sector * disk->sector_size
                       + cluster * FAT16_FAT_ENTRY_SIZE);
      int res = disk_stream_write (private->fat_read_stream, &value,
                                   sizeof (value));
      if (res < 0)
        {
          return res;
        }
    }
  return OK;
}

// finds a free cluster, marks it as the end of a chain and zeroes it.
// returns the cluster or a negative error when the disk is full
static int
fat16_alloc_cluster (struct disk *disk)
{
  struct fat_private *private = disk->fs_private_data;
  uint32_t limit = fat16_cluster_limit (disk);
  uint32_t start = private->next_free_cluster;
  if (start < 2 || start >= limit)
    {
      start = 2;
    }
  for (uint32_t n = 0; n < limit - 2; n++)
    {
      uint32_t cluster = start + n;
      if (cluster >= limit)
        {
          cluster -= limit - 2;
        }
      int entry = fat16_get_fat_entry (disk, cluster);
      if (entry < 0)
        {
          return entry;
        }
      if (entry != FAT16_UNUSED)
        {
          continue;
        }
      int res = fat16_set_fat_entry (disk, cluster, 0xFFFF);
      if (res < 0)
        {
          return res;
        }
      // directories rely on zeroed clusters, the end marker is a zero byte
      char zero[512];
      memset (zero, 0, sizeof (zero));
      stream_seek (private->cluster_read_stream,
                   fat16_cluster_to_sector (private, cluster)
                       * disk->sector_size);
      for (uint32_t done = 0; done < fat16_cluster_bytes (disk);
           done += sizeof (zero))
        {
          disk_stream_write (private->cluster_read_stream, zero,
                             sizeof (zero));
        }
    private
      ->next_free_cluster = cluster + 1;
      return cluster;
    }
  return -NO_MEMORY_ERROR;
}

static void
fat16_free_chain (struct disk *disk, uint32_t cluster)
{
  while (cluster >= 2 && cluster < FAT16_RESERVED_START)
    {
      int next = fat16_get_fat_entry (disk, cluster);
      fat16_set_fat_entry (disk, cluster, FAT16_UNUSED);
      if (next < 0)
        {
          return;
        }
      cluster = next;
    }
}

// returns the index-th cluster of the chain starting at *first. with allocate
// set, missing clusters are appended and an empty chain gets a first cluster
static int
fat16_cluster_at (struct disk *disk, uint32_t *first, uint32_t index,
                  uint8_t allocate)
{
  if (*first == 0)
    {
      if (!allocate)
        {
          return -IO_ERROR;
        }
      int cluster = fat16_alloc_cluster (disk);
      if (cluster < 0)
        {
          return cluster;
        }
      *first = cluster;
    }
  uint32_t cluster = *first;
  for (uint32_t i = 0; i < index; i++)
    {
      int next = fat16_get_fat_entry (disk, cluster);
      if (next < 0)
        {
          return next;
        }
      if (next >= FAT16_END_OF_CHAIN)
        {
          if (!allocate)
            {
              return -IO_ERROR;
            }
          next = fat16_alloc_cluster (disk);
          if (next < 0)
            {
              return next;
            }
          fat16_set_fat_entry (disk, cluster, next);
        }
      else if (next < 2 || next >= FAT16_RESERVED_START)
        {
          return -IO_ERROR;
        }
      cluster = next;
    }
  return cluster;
}

static void
fat16_set_first_cluster (struct fat_directory_item *item, uint32_t cluster)
{
  item->low_16_bits_first_cluster = cluster & 0xFFFF;
  item->high_16_bits_first_cluster = cluster >> 16;
}

// absolute byte position of item idx of directory on disk
static int
fat16_dir_entry_pos (struct disk *disk, struct fat_directory *directory,
                     int idx)
{
  struct fat_private *private = disk->fs_private_data;
  uint32_t offset = idx * sizeof (struct fat_directory_item);
  if (directory->first_cluster == 0)
    {
      return directory->sector_pos * disk->sector_size + offset;
    }
  int cluster
      = fat16_get_cluster_for_offset (disk, directory->first_cluster, offset);
  if (cluster < 0)
    {
      return cluster;
    }
  return fat16_cluster_to_sector (private, cluster) * disk->sector_size
         + offset % fat16_cluster_bytes (disk);
}

// writes a directory item to disk and keeps the cached root directory in sync
static int
fat16_write_dir_entry (struct disk *disk, uint32_t pos,
                       struct fat_directory_item *item)
{
  struct fat_private *private = disk->fs_private_data;
  stream_seek (private->directory_stream, pos);
  int res = disk_stream_write (private->directory_stream, item,
                               sizeof (struct fat_directory_item));
  if (res < 0)
    {
      return res;
    }
  struct fat_directory *root = &private->root_directory;
  uint32_t root_start = root->sector_pos * disk->sector_size;
  uint32_t root_entries = private->header.primary_header.root_dir_entries;
  if (pos >= root_start
      && pos < root_start + root_entries * sizeof (struct fat_directory_item))
    {
      int idx = (pos - root_start) / sizeof (struct fat_directory_item);
      root->item[idx] = *item;
      if (item->filename[0] != FAT_ENTRY_END && idx >= root->total)
        {
          root->total = idx + 1;
        }
    }
  return OK;
}

// returns an index where a new item can be stored in directory. full
// subdirectories grow by a cluster, the root directory has a fixed size
static int
fat16_find_free_slot (struct disk *disk, struct fat_directory *directory)
{
  struct fat_private *private = disk->fs_private_data;
  for (int i = 0; i < directory->total; i++)
    {
      if (directory->item[i].filename[0] == FAT_ENTRY_DELETED)
        {
          return i;
        }
    }
  int idx = directory->total;
  if (directory->first_cluster == 0)
    {
      // keep one end marker after the last item
      return idx + 1 < private->header.primary_header.root_dir_entries
                 ? idx
                 : -NO_MEMORY_ERROR;
    }
  // appends a cluster to the directory if idx (or the end marker after it)
  // is past its last cluster. new clusters are zeroed, so they end the list
  uint32_t per_cluster
      = fat16_cluster_bytes (disk) / sizeof (struct fat_directory_item);
  uint32_t first = directory->first_cluster;
  int res = fat16_cluster_at (disk, &first, (idx + 1) / per_cluster, 1);
  return res < 0 ? res : idx;
}

// sets the creation and modification time of a new item to now
static void
fat16_stamp_item (struct fat_directory_item *item)
{
  item->creation_date = rtc_fat_date ();
  item->creation_time = rtc_fat_time ();
  item->last_mod_date = item->creation_date;
  item->last_mod_time = item->creation_time;
  item->last_access = item->creation_date;
}

// converts "notes.txt" to the padded uppercase 8.3 form "NOTES   TXT"
static int
fat16_make_short_name (const char *name, uint8_t *out)
{
  const char *allowed = "!#$%&'()-@^_`{}~";
  memset (out, ' ', 11);
  const char *dot = 0;
  for (const char *c = name; *c; c++)
    {
      if (*c == '.')
        {
          dot = c;
        }
    }
  uint32_t base_len = dot ? (uint32_t)(dot - name) : strlen (name);
  uint32_t ext_len = dot ? strlen (dot + 1) : 0;
  if (base_len == 0 || base_len > 8 || ext_len > 3 || (dot && ext_len == 0))
    {
      return -INVALID_PATH_ERROR;
    }
  for (uint32_t i = 0; i < base_len + (dot ? ext_len + 1 : 0); i++)
    {
      char c = name[i];
      if (name + i == dot)
        {
          continue;
        }
      if (!isalnum (c) && !strchr (allowed, c))
        {
          return -INVALID_PATH_ERROR;
        }
      uint32_t pos = (dot && name + i > dot) ? 8 + (name + i - dot - 1) : i;
      out[pos] = toupper (c);
    }
  return OK;
}

// splits path into its parent directory and last part. the parent must be
// released with fat16_release_parent
struct fat16_parent
{
  struct fat_directory *directory;
  struct fat_item *owner; // loaded subdirectory, 0 for the root directory
  const char *name;
};

static int
fat16_get_parent (struct disk *disk, struct path_part *path,
                  struct fat16_parent *out)
{
  struct fat_private *private = disk->fs_private_data;
  if (!path)
    {
      return -INVALID_PATH_ERROR;
    }
  out->owner = 0;
  if (!path->next)
    {
      out->directory = &private->root_directory;
      out->name = path->part;
      return OK;
    }
  struct path_part *before_last = path;
  while (before_last->next->next)
    {
      before_last = before_last->next;
    }
  struct path_part *last = before_last->next;
  // look up the path without its last part
  before_last->next = 0;
  struct fat_item *item = fat16_get_directory_entry (disk, path);
  before_last->next = last;
  if (!item)
    {
      return -NOT_FOUND_ERROR;
    }
  if (item->type != FAT_ITEM_TYPE_DIRECTORY)
    {
      fat16_fat_item_free (item);
      return -NOT_DIRECTORY_ERROR;
    }
  out->directory = item->directory;
  out->owner = item;
  out->name = last->part;
  return OK;
}

static void
fat16_release_parent (struct fat16_parent *parent)
{
  if (parent->owner)
    {
      fat16_fat_item_free (parent->owner);
    }
}

// adds a new item to the parent directory and returns its disk position
static int
fat16_create_item (struct disk *disk, struct fat16_parent *parent,
                   struct fat_directory_item *item)
{
  int slot = fat16_find_free_slot (disk, parent->directory);
  if (slot < 0)
    {
      return slot;
    }
  int pos = fat16_dir_entry_pos (disk, parent->directory, slot);
  if (pos < 0)
    {
      return pos;
    }
  int res = fat16_write_dir_entry (disk, pos, item);
  return res < 0 ? res : pos;
}

static void *
fat16_open_for_write (struct disk *disk, struct path_part *path,
                      FILE_MODE mode)
{
  struct fat16_parent parent;
  int res = fat16_get_parent (disk, path, &parent);
  if (res < 0)
    {
      return ERROR (res);
    }

  struct fat_directory_item entry;
  int pos = 0;
  int idx = fat16_find_index (parent.directory, parent.name);
  if (idx >= 0)
    {
      entry = parent.directory->item[idx];
      if (entry.attribute & FAT_FILE_SUBDIRECTORY)
        {
          res = -INVALID_ARG_ERROR;
          goto out;
        }
      pos = fat16_dir_entry_pos (disk, parent.directory, idx);
    }
  else
    {
      memset (&entry, 0, sizeof (entry));
      res = fat16_make_short_name (parent.name, entry.filename);
      if (res < 0)
        {
          goto out;
        }
      entry.attribute = FAT_FILE_ARCHIVED;
      fat16_stamp_item (&entry);
      pos = fat16_create_item (disk, &parent, &entry);
    }
  if (pos < 0)
    {
      res = pos;
      goto out;
    }

  // "w" starts with an empty file
  if (mode == FILE_MODE_WRITE && fat16_get_first_cluster (&entry))
    {
      fat16_free_chain (disk, fat16_get_first_cluster (&entry));
      fat16_set_first_cluster (&entry, 0);
      entry.filesize = 0;
      res = fat16_write_dir_entry (disk, pos, &entry);
      if (res < 0)
        {
          goto out;
        }
    }

  struct fat_file_descriptor *descriptor
      = calloc (sizeof (struct fat_file_descriptor));
  struct fat_item *f_item = calloc (sizeof (struct fat_item));
  struct fat_directory_item *item_copy
      = fat16_clone_directory_item (&entry, sizeof (entry));
  if (!descriptor || !f_item || !item_copy)
    {
      free (descriptor);
      free (f_item);
      free (item_copy);
      res = -NO_MEMORY_ERROR;
      goto out;
    }
  f_item->type = FAT_ITEM_TYPE_FILE;
  f_item->item = item_copy;
  descriptor->item = f_item;
  descriptor->mode = mode;
  descriptor->dir_entry_pos = pos;
  descriptor->pos = mode == FILE_MODE_APPEND ? entry.filesize : 0;
  fat16_release_parent (&parent);
  return descriptor;

out:
  fat16_release_parent (&parent);
  return ERROR (res);
}

int
fat16_write (struct disk *disk, void *descriptor, uint32_t size,
             uint32_t nmemb, const char *in)
{
  struct fat_private *private = disk->fs_private_data;
  struct fat_file_descriptor *desc = descriptor;
  if (desc->mode == FILE_MODE_READ || desc->item->type != FAT_ITEM_TYPE_FILE)
    {
      return -INVALID_ARG_ERROR;
    }
  struct fat_directory_item *item = desc->item->item;
  uint32_t cluster_bytes = fat16_cluster_bytes (disk);
  uint32_t total = size * nmemb;
  uint32_t written = 0;
  int res = OK;
  while (written < total)
    {
      uint32_t first = fat16_get_first_cluster (item);
      int cluster
          = fat16_cluster_at (disk, &first, desc->pos / cluster_bytes, 1);
      fat16_set_first_cluster (item, first);
      if (cluster < 0)
        {
          res = cluster;
          break;
        }
      uint32_t offset = desc->pos % cluster_bytes;
      uint32_t chunk = cluster_bytes - offset;
      if (chunk > total - written)
        {
          chunk = total - written;
        }
      stream_seek (private->cluster_read_stream,
                   fat16_cluster_to_sector (private, cluster)
                           * disk->sector_size
                       + offset);
      res = disk_stream_write (private->cluster_read_stream, in + written,
                               chunk);
      if (res < 0)
        {
          break;
        }
      written += chunk;
      desc->pos += chunk;
    }
  if (desc->pos > item->filesize)
    {
      item->filesize = desc->pos;
    }
  item->last_mod_date = rtc_fat_date ();
  item->last_mod_time = rtc_fat_time ();
  item->last_access = item->last_mod_date;
  int dir_res = fat16_write_dir_entry (disk, desc->dir_entry_pos, item);
  if (res < 0 || dir_res < 0)
    {
      return res < 0 ? res : dir_res;
    }
  return nmemb;
}

static int
fat16_directory_is_empty (struct disk *disk, struct fat_directory_item *item)
{
  struct fat_directory *directory = fat16_load_fat_directory (disk, item);
  if (!directory)
    {
      // a directory without any items can fail to load
      return 1;
    }
  int empty = 1;
  for (int i = 0; i < directory->total; i++)
    {
      uint8_t first = directory->item[i].filename[0];
      if (first != FAT_ENTRY_DELETED && first != '.'
          && directory->item[i].attribute != FAT_FILE_LONG_NAME)
        {
          empty = 0;
        }
    }
  fat16_free_directory (directory);
  return empty;
}

int
fat16_remove (struct disk *disk, struct path_part *path)
{
  struct fat16_parent parent;
  int res = fat16_get_parent (disk, path, &parent);
  if (res < 0)
    {
      return res;
    }
  int idx = fat16_find_index (parent.directory, parent.name);
  if (idx < 0)
    {
      res = -NOT_FOUND_ERROR;
      goto out;
    }
  struct fat_directory_item item = parent.directory->item[idx];
  if ((item.attribute & FAT_FILE_SUBDIRECTORY)
      && !fat16_directory_is_empty (disk, &item))
    {
      res = -DIR_NOT_EMPTY_ERROR;
      goto out;
    }
  int pos = fat16_dir_entry_pos (disk, parent.directory, idx);
  if (pos < 0)
    {
      res = pos;
      goto out;
    }
  fat16_free_chain (disk, fat16_get_first_cluster (&item));
  item.filename[0] = FAT_ENTRY_DELETED;
  res = fat16_write_dir_entry (disk, pos, &item);
out:
  fat16_release_parent (&parent);
  return res;
}

int
fat16_mkdir (struct disk *disk, struct path_part *path)
{
  struct fat_private *private = disk->fs_private_data;
  struct fat16_parent parent;
  int res = fat16_get_parent (disk, path, &parent);
  if (res < 0)
    {
      return res;
    }
  struct fat_directory_item entry;
  memset (&entry, 0, sizeof (entry));
  if (fat16_find_index (parent.directory, parent.name) >= 0)
    {
      res = -FILE_EXISTS_ERROR;
      goto out;
    }
  res = fat16_make_short_name (parent.name, entry.filename);
  if (res < 0)
    {
      goto out;
    }
  int cluster = fat16_alloc_cluster (disk);
  if (cluster < 0)
    {
      res = cluster;
      goto out;
    }
  entry.attribute = FAT_FILE_SUBDIRECTORY;
  fat16_set_first_cluster (&entry, cluster);
  fat16_stamp_item (&entry);

  // every directory starts with "." (itself) and ".." (its parent, 0 for
  // the root directory)
  struct fat_directory_item dots[2];
  memset (dots, 0, sizeof (dots));
  memset (dots[0].filename, ' ', 11);
  memset (dots[1].filename, ' ', 11);
  dots[0].filename[0] = '.';
  dots[1].filename[0] = '.';
  dots[1].filename[1] = '.';
  dots[0].attribute = FAT_FILE_SUBDIRECTORY;
  dots[1].attribute = FAT_FILE_SUBDIRECTORY;
  fat16_stamp_item (&dots[0]);
  fat16_stamp_item (&dots[1]);
  fat16_set_first_cluster (&dots[0], cluster);
  fat16_set_first_cluster (&dots[1], parent.directory->first_cluster);
  stream_seek (private->cluster_read_stream,
               fat16_cluster_to_sector (private, cluster) * disk->sector_size);
  res = disk_stream_write (private->cluster_read_stream, dots, sizeof (dots));
  if (res < 0)
    {
      goto out;
    }
  res = fat16_create_item (disk, &parent, &entry);
  if (res < 0)
    {
      fat16_free_chain (disk, cluster);
    }
  else
    {
      res = OK;
    }
out:
  fat16_release_parent (&parent);
  return res;
}
