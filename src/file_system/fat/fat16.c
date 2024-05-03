#include "fat16.h"
#include "../../drivers/disk/disk_stream.h"
#include "../../kernel/config.h"
#include "../../kernel/kpanic.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/stdlib/stdlib.h"
#include "../../libc/string/string.h"
#include "../file.h"

// all code written here is based of Microsoft Extensible Firmware Initiative
// FAT32 File System Specification
//  and NOT A FAT16 MANUAL. TODO: DOUBLE CHECK WITH A FAT16 MANUAL

#include <stdint.h>

#define FAT16_SIGNATURE 0x29
#define FAT16_FAT_ENTRY 0x02
#define FAT16_BAD_SECTOR 0xff7
#define FAT16_UNUSED 0x00

typedef uint8_t FAT16_ITEM_TYPE;
#define FAT16_ITEM_TYPE_DIR 0
#define FAT16_ITEM_TYPE_FILE 1

// fat dir entry attributes bitmap
#define FAT16_FILE_READ_ONLY 0x01
#define FAT16_FILE_HIDDEN 0X02
#define FAT16_FILE_SYSTEM 0X04
#define FAT16_FILE_VOLUME_LABEL 0X08
#define FAT16_FILE_SUB_DIR 0X10
#define FAT16_FILE_ARCHIVED 0X20
#define FAT16_FILE_DEVICE 0X40
#define FAT16_FILE_RESERVED 0X80

struct FAT_header_extended
{
  uint8_t drive_number;
  uint8_t win_nt_bit;
  uint8_t signature;
  uint32_t volume_id;
  uint8_t volume_id_str[11];
  uint8_t sys_id_str[8];
} __attribute__ ((packed));

struct FAT_h
{
  uint8_t short_jmp_inst[3];
  uint8_t oem_identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t FAT_copies;
  uint16_t root_dir_entries;
  uint16_t number_of_sectors;
  uint8_t media_type;
  uint16_t sectors_per_FAT;
  uint16_t sectors_per_track;
  uint16_t number_of_heads;
  uint32_t hidden_sectors;
  uint32_t sectors_big;
} __attribute__ ((packed));

struct FAT_header
{
  struct FAT_h primary_header;
  // extended header is optional so union it is
  union fat_h_e
  {
    struct FAT_header_extended extender_header;
  } shared;
};

struct FAT_dir_item
{
  uint8_t filename[8];
  uint8_t ext[3];
  uint8_t attr;
  uint8_t reserved;
  uint8_t creation_time_tenths_of_a_sec;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_accessed;
  uint16_t high_16_bit_first_cluster;
  uint16_t last_modf_time;
  uint16_t last_modf_date;
  uint16_t low_16_bit_first_cluster;
  uint32_t file_size;
} __attribute__ ((packed));

// fat16 dir wrapper
struct FAT_directory
{
  struct FAT_dir_item *item;
  int32_t total;
  int32_t sector_start_pos;
  int32_t sector_end_pos;
};

struct FAT_item
{
  union
  {
    struct FAT_dir_item *item;
    struct FAT_directory *dir;
  };
  FAT16_ITEM_TYPE type;
};

// represents an open file
struct FAT_file_desc
{
  struct FAT_item *item;
  uint32_t pos;
};

struct FAT_private
{
  struct FAT_header header;
  struct FAT_directory root_dir;
  struct disk_stream *cluster_read_stream;
  struct disk_stream *FAT_read_stream;
  struct disk_stream *dir_stream;
};

struct file_system fat16_fs = {
  .resolve_fn = fat16_resolve,
  .open_fn = fat16_open,
};

struct file_system *
fat16_init ()
{
  memset (fat16_fs.fs_name, 'a', 20);
  strcpy (fat16_fs.fs_name, "FAT16");
  return &fat16_fs;
}

static void
fat16_init_private (struct disk_t *disk, struct FAT_private *private)
{
  memset (private, 0, sizeof (struct FAT_private));
  private->cluster_read_stream = get_disk_stream (disk->id);
  private->FAT_read_stream = get_disk_stream (disk->id);
  private->dir_stream = get_disk_stream (disk->id);
}

// counts total used dir in a directory
static int32_t
fat16_total_items_in_dir (struct disk_t *disk, uint32_t dir_start_sec)
{
  struct FAT_dir_item item;
  struct FAT_dir_item empty_item;
  memset (&empty_item, 0, sizeof (empty_item));
  struct FAT_private *private = disk->fs_private_data;
  int32_t dir_start_pos = dir_start_sec * disk->sector_size;
  struct disk_stream *stream = private->dir_stream;
  stream_seek (stream, dir_start_pos);
  size_t item_count = 0;
  while (1)
    {
      if (disk_stream_read (stream, &item, sizeof (item)) != OK)
        {
          return -IO_ERROR;
        }
      if (item.filename[0] == 0)
        {
          // reading is done if items name is null
          break;
        }
      if (item.filename[0] == 0xe5)
        {
          // continue if item is unused
          continue;
        }
      item_count++;
    }
  return item_count;
}

static int32_t
fat16_get_root_dir (struct disk_t *disk, struct FAT_private *private,
                    struct FAT_directory *fat_dir)
{
  struct FAT_h *primary_header = &private->header.primary_header;
  int32_t root_dir_pos
      = (primary_header->FAT_copies * primary_header->sectors_per_FAT)
        + primary_header->reserved_sectors;
  int32_t root_dir_size = private->header.primary_header.root_dir_entries
                          * sizeof (struct FAT_dir_item);
  int32_t total_sectors = root_dir_size / disk->sector_size;
  // check if there is any excess sector
  total_sectors = (root_dir_size % disk->sector_size) ? total_sectors + 1
                                                      : total_sectors;
  int32_t total_items = fat16_total_items_in_dir (disk, root_dir_pos);
  if (total_items < 0)
    {
      // at this point, total_items holds the error code
      return total_items;
    }
  struct FAT_dir_item *dir = calloc (root_dir_size);
  if (!dir)
    {
      return -NO_MEMORY_ERROR;
    }
  struct disk_stream *stream = private->dir_stream;
  // set stream to the absolute address on the disk
  stream_seek (stream, disk->sector_size * root_dir_pos);
  if (disk_stream_read (stream, dir, root_dir_size) != OK)
    {
      return -IO_ERROR;
    }
  fat_dir->item = dir;
  fat_dir->total = total_items;
  fat_dir->sector_start_pos = root_dir_pos;
  fat_dir->sector_end_pos = root_dir_pos + (root_dir_size / disk->sector_size);
  return OK;
}

int32_t
fat16_resolve (struct disk_t *disk)
{
  int32_t rs = OK;
  struct FAT_private *private = calloc (sizeof (struct FAT_private));
  fat16_init_private (disk, private);
  disk->fs_private_data = private;
  disk->file_system = &fat16_fs;
  struct disk_stream *stream = get_disk_stream (disk->id);
  if (!stream)
    {
      rs = -NO_MEMORY_ERROR;
      goto end;
    }
  if (disk_stream_read (stream, &private->header, sizeof (private->header))
      != OK)
    {
      rs = -IO_ERROR;
      goto end;
    }
  if (private->header.shared.extender_header.signature != 0x29)
    {
      rs = -SIGNATURE_ERROR;
      goto end;
    }
  if (fat16_get_root_dir (disk, private, &private->root_dir) != OK)
    {
      rs = -IO_ERROR;
    }

end:
  if (rs < 0)
    {
      free (private);
      disk->fs_private_data = 0;
    }
  if (stream)
    {
      stream_free (stream);
    }
  return rs;
}

// fat16 allows space in the paths such as:
// 0:/FILE    /PATH8BYTE/realfile
// each file has a 8 byte long names. if file's name doesnt fill the buffer
// space chars will. so this functions strips that space away
static void
format_file_name (char **buffer, const char *input)
{
  while (*input != 0x00 || *input != 0x20)
    {
      **buffer = *input;
      (*buffer)++;
      input++;
    }
  if (*input == 0x20)
    {
      **buffer = 0x00;
    }
}

static void
get_relative_file_name (struct FAT_dir_item *item, char *buffer, size_t size)
{
  memset (buffer, 0, size);
  // strip space chars in filename and sets buffer to it.
  format_file_name (&buffer, (const char *)item->filename);
  // add .extension after the buffer if extension for the given file exists
  if (item->ext[0] != 0x00 && item->ext[0] != 0x20)
    {
      *buffer++ = '.';
      format_file_name (&buffer, (const char *)item->ext);
    }
}

static struct FAT_dir_item *
clone_dir_item (struct FAT_dir_item *item, size_t size)
{
  if (size < sizeof (struct FAT_dir_item))
    {
      return 0;
    }
  struct FAT_dir_item *copy = calloc (size);
  if (!copy)
    {
      return copy;
    }
  memcpy (copy, item, size);
  return copy;
}

static int32_t
get_fat_entry (struct disk_t *disk, int32_t cluster_to_use)
{
  struct FAT_private *priv = disk->fs_private_data;
  struct disk_stream *stream = priv->FAT_read_stream;
  if (!stream)
    {
      return -IO_ERROR;
    }
  uint32_t fat_pos
      = priv->header.primary_header.reserved_sectors * disk->sector_size;
  // set stream to the start of file's position
  stream_seek (stream, fat_pos * cluster_to_use * FAT16_FAT_ENTRY);
  int32_t result = 0;
  int32_t rs = disk_stream_read (stream, &result, sizeof (result));
  if (rs < 0)
    {
      return -IO_ERROR;
    }
  return result;
}

// get the correct cluster based on start and offset
static int32_t
get_cluster_for_offset (struct disk_t *disk, int32_t start, int32_t offset,
                        int32_t size_of_cluster)
{
  // struct FAT_private *priv = disk->fs_private_data;
  int32_t cluster_to_use = start;
  int32_t clusters_ahead = offset / size_of_cluster;
  for (size_t i = 0; i < clusters_ahead; i++)
    {
      int32_t entry = get_fat_entry (disk, cluster_to_use);
      // if entry is one of them, then entry is the last one on the fat
      if (entry == 0xFF8 || entry == 0xFFF)
        {
          return -IO_ERROR;
        }
      if (entry == FAT16_BAD_SECTOR)
        {
          return -IO_ERROR;
        }
      if (entry == 0XFF0 || entry == 0xFF6)
        {
          return -IO_ERROR;
        }
      if (entry == 0x00)
        {
          return -IO_ERROR;
        }
      cluster_to_use = entry;
    }
  return cluster_to_use;
}

// internal read function for reading fat table entries until current file ends
static int32_t
fat16_read_internal (struct disk_t *disk, int32_t cluster, int32_t offset,
                     int32_t total, void *buffer)
{
  struct FAT_private *priv = disk->fs_private_data;
  struct disk_stream *stream = priv->cluster_read_stream;
  int32_t size_of_cluster
      = priv->header.primary_header.sectors_per_cluster * disk->sector_size;
  int32_t cluster_to_use
      = get_cluster_for_offset (disk, cluster, offset, size_of_cluster);
  if (cluster_to_use < 0)
    {
      return cluster_to_use;
    }
  int32_t offset_from_cluster = offset % size_of_cluster;
  int32_t start_sector
      = priv->root_dir.sector_end_pos
        + ((cluster - 2) * priv->header.primary_header.sectors_per_cluster);
  int32_t start_pos = (start_sector * disk->sector_size) * offset_from_cluster;
  int32_t total_to_read = total > size_of_cluster ? size_of_cluster : total;
  stream_seek (stream, start_pos);
  int32_t rs = disk_stream_read (stream, buffer, total_to_read);
  if (rs != OK)
    {
      return -IO_ERROR;
    }
  total -= total_to_read;
  if (total > 0)
    {
      fat16_read_internal (disk, cluster, offset + total_to_read, total,
                           buffer + total_to_read);
    }
  return 0;
}

void
fat16_free_dir (struct FAT_directory *dir)
{
  if (!dir)
    {
      return;
    }
  if (dir->item)
    {
      free (dir->item);
    }
  free (dir);
}

static struct FAT_directory *
load_fat_dir (struct disk_t *disk, struct FAT_dir_item *item)
{
  struct FAT_private *priv = disk->fs_private_data;
  // return null if given file is not a directory
  if (!(item->attr & FAT16_FILE_SUB_DIR))
    {
      return 0;
    }
  struct FAT_directory *dir = calloc (sizeof (struct FAT_directory));
  if (!dir)
    {
      return 0;
    }
  // join high 16 and low 16 for full cluster addr
  int32_t cluster
      = item->high_16_bit_first_cluster | item->low_16_bit_first_cluster;
  int32_t cluster_sector
      = priv->root_dir.sector_end_pos
        + ((cluster - 2) * priv->header.primary_header.sectors_per_cluster);
  dir->total = fat16_total_items_in_dir (disk, cluster_sector);
  // a directory is an array of dir items
  int32_t dir_size = dir->total * sizeof (struct FAT_dir_item);
  dir->item = calloc (dir_size);
  if (!dir->item)
    {
      goto end;
    }
  int32_t rs = fat16_read_internal (disk, cluster, 0x00, dir_size, dir->item);
  if (!rs)
    {
      goto end;
    }
end:
  if (dir)
    {
      fat16_free_dir (dir);
    }
  return dir;
}

static struct FAT_item *
fat_item_init (struct disk_t *disk, struct FAT_dir_item *item)
{
  // a FAT item is either a director or a file
  struct FAT_item *f_item = calloc (sizeof (struct FAT_item));
  if (!f_item)
    {
      return 0;
    }
  // check if given item is a directory or not
  if (item->attr & FAT16_FILE_SUB_DIR)
    {
      f_item->dir = load_fat_dir (disk, item);
      f_item->type = FAT16_ITEM_TYPE_DIR;
    }
  else
    {
      f_item->type = FAT16_ITEM_TYPE_FILE;
      f_item->item = clone_dir_item (item, sizeof (struct FAT_dir_item));
    }
  return f_item;
}

// iterates in a dictionary to find the desired path
static struct FAT_item *
find_item_in_dir (struct disk_t *disk, struct FAT_directory *dir,
                  const char *path_part)
{
  struct FAT_item *fat_item = 0;
  char temp_file_name[MAX_PATH_LEN];
  // loop through all items in a directory
  for (size_t i = 0; i < dir->total; i++)
    {
      get_relative_file_name (&dir->item[i], temp_file_name,
                              sizeof (temp_file_name));
      // check if file name matches to a file inside dir
      if (!strncmp (temp_file_name, path_part, sizeof (temp_file_name)))
        {
          fat_item = fat_item_init (disk, &dir->item[i]);
        }
    }
  return fat_item;
}

void
fat16_item_free (struct FAT_item *item)
{
  if (item->type == FAT16_ITEM_TYPE_DIR)
    {
      fat16_free_dir (item->dir);
    }
  else if (item->type == FAT16_ITEM_TYPE_FILE)
    {
      free (item->item);
    }
  else
    {
      panic ("no free fat16 table directory");
    }
  free (item);
}

// find the file on the given disk. returns null if cant find
static struct FAT_item *
get_dir_entry (struct disk_t *disk, struct path_body *body)
{
  struct FAT_private *priv = disk->fs_private_data;
  struct FAT_item *current_item = 0;
  // get the root item for given path
  struct FAT_item *root_item
      = find_item_in_dir (disk, &priv->root_dir, body->body_str);
  if (!root_item)
    {
      return current_item;
    }
  struct path_body *next_part = body->next;
  current_item = root_item;
  // if there are more items to get, get them until none left
  while (next_part != 0)
    {
      if (current_item->type != FAT16_ITEM_TYPE_DIR)
        {
          current_item = 0;
          break;
        }
      struct FAT_item *tmp_item
          = find_item_in_dir (disk, current_item->dir, next_part->body_str);
      fat16_item_free (current_item);
      current_item = tmp_item;
      next_part = next_part->next;
    }
  return current_item;
}

void *
fat16_open (struct disk_t *disk, struct path_root *root, enum FILE_MODE mode)
{
  struct FAT_file_desc *desc = calloc (sizeof (struct FAT_file_desc));
  if (!desc)
    {
      return ERROR (-NO_MEMORY_ERROR);
    }
  desc->item = get_dir_entry (disk, root->body);
  desc->pos = 0;
  return desc;
}
