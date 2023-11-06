#include "../file.h"
#include "fat16.h"
#include "../../libc/string/string.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/stdlib/stdlib.h"
#include "../../kernel/config.h"
#include "../../drivers/disk/disk_stream.h"

//all code written here is based of Microsoft Extensible Firmware Initiative FAT32 File System Specification
// and NOT A FAT16 MANUAL. TODO: DOUBLE CHECK WITH A FAT16 MANUAL
#include <stdint.h>

#define FAT16_SIGNATURE  0x29
#define FAT16_FAT_ENTRY  0x02
#define FAT16_BAD_SECTOR 0xff7
#define FAT16_UNUSED     0x00

typedef uint8_t FAT16_ITEM_TYPE;
#define FAT16_ITEM_TYPE_DIR 0
#define FAT16_ITEM_TYPE_FILE 1

// fat dir entry attributes bitmap
#define FAT16_FILE_READ_ONLY    0x01
#define FAT16_FILE_HIDDEN       0X02
#define FAT16_FILE_SYSTEM       0X04
#define FAT16_FILE_VOLUME_LABEL 0X08
#define FAT16_FILE_SUB_DIR      0X10 
#define FAT16_FILE_ARCHIVED     0X20 
#define FAT16_FILE_DEVICE       0X40
#define FAT16_FILE_RESERVED     0X80

struct FAT_header_extended{
  uint8_t drive_number;
  uint8_t win_nt_bit;
  uint8_t signature;
  uint32_t volume_id;
  uint8_t volume_id_str[11];
  uint8_t sys_id_str[8];
}__attribute__((packed));

struct FAT_h{
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
}__attribute__((packed));

struct FAT_header{
  struct FAT_h primary_header;
  // extended header is optional so union it is
  union fat_h_e{
    struct FAT_header_extended extender_header;
  }shared;
};

struct FAT_dir_item{
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
}__attribute__((packed));

// fat16 dir wrapper
struct FAT_directory{
  struct FAT_dir_item *item;
  int32_t total;
  int32_t sector_start_pos;
  int32_t sector_end_pos; 

};

struct FAT_item{
  union{
    struct FAT_dir_item *item;
    struct FAT_directory *dir;
  };
  FAT16_ITEM_TYPE type;
};

// represents an open file
struct FAT_item_desc{
  struct FAT_item *item;
  uint32_t pos;
};

struct FAT_private{
  struct FAT_header header;
  struct FAT_directory root_dir;
  struct disk_stream *cluster_read_stream;
  struct disk_stream *FAT_read_stream;
  struct disk_stream *dir_stream;
};

struct file_system fat16_fs ={
  .resolve_fn = fat16_resolve,
  .open_fn = fat16_open,
};

struct file_system *fat16_init(){
  memset(fat16_fs.fs_name, 'a', 20);
  strcpy(fat16_fs.fs_name, "FAT16");
  return &fat16_fs;
}

static void fat16_init_private(struct disk_t *disk, struct FAT_private *private){
  memset(private, 0, sizeof(struct FAT_private));
  private->cluster_read_stream = get_disk_stream(disk->id);
  private->FAT_read_stream= get_disk_stream(disk->id);
  private->dir_stream = get_disk_stream(disk->id);
}

// counts total used dir in a directory
static int32_t fat16_total_items_in_dir(struct disk_t *disk, uint32_t dir_start_sec){
  struct FAT_dir_item item;
  struct FAT_dir_item empty_item;
  memset(&empty_item,0,sizeof(empty_item));
  struct FAT_private *private = disk->fs_private_data;
  int32_t dir_start_pos = dir_start_sec * disk->sector_size;
  struct disk_stream *stream = private->dir_stream;
  stream_seek(stream, dir_start_pos);
  size_t item_count = 0;
  while(1){
    if(disk_stream_read(stream, &item, sizeof(item)) != OK){
      return -IO_ERROR;
    }
    if(item.filename[0] == 0){
      // reading is done if items name is null
      break;
    }
    if(item.filename[0] == 0xe5){
      // continue if item is unused
      continue;
    }
    item_count++;
  }
  return item_count;
}

static int32_t fat16_get_root_dir(struct disk_t *disk, struct FAT_private *private, struct FAT_directory *fat_dir){
  struct FAT_h *primary_header = &private->header.primary_header;
  int32_t root_dir_pos = (primary_header->FAT_copies * primary_header->sectors_per_FAT) + primary_header->reserved_sectors; 
  int32_t root_dir_size = private->header.primary_header.root_dir_entries * sizeof(struct FAT_dir_item);
  int32_t total_sectors = root_dir_size / disk->sector_size;
  // check if there is any excess sector 
  total_sectors = (root_dir_size % disk->sector_size) ? total_sectors + 1 : total_sectors;
  int32_t total_items = fat16_total_items_in_dir(disk, root_dir_pos);  
  if(total_items < 0){
    // at this point, total_items holds the error code
    return total_items;
  }
  struct FAT_dir_item *dir = calloc(root_dir_size);
  if(!dir){
    return -NO_MEMORY_ERROR;
  }
  struct disk_stream *stream = private->dir_stream;
  // set stream to the absolute address on the disk
  stream_seek(stream, disk->sector_size * root_dir_pos);
  if(disk_stream_read(stream, dir, root_dir_size) != OK){
    return -IO_ERROR;
  }
  fat_dir->item = dir;
  fat_dir->total = total_items;
  fat_dir->sector_start_pos = root_dir_pos;
  fat_dir->sector_end_pos = root_dir_pos + (root_dir_size / disk->sector_size);
  return OK;
}

int32_t fat16_resolve(struct disk_t *disk){
  int32_t rs = OK;
  struct FAT_private *private = calloc(sizeof(struct FAT_private));
  fat16_init_private(disk, private);
  disk->fs_private_data = private;
  disk->file_system = &fat16_fs;
  struct disk_stream *stream = get_disk_stream(disk->id);
  if(!stream){
    rs =  -NO_MEMORY_ERROR;
    goto end;
  }
  if(disk_stream_read(stream, &private->header, sizeof(private->header)) != OK){
    rs = -IO_ERROR;
    goto end;
  }
  if(private->header.shared.extender_header.signature != 0x29){
    rs =  -SIGNATURE_ERROR;
    goto end;
  }
  if(fat16_get_root_dir(disk, private, &private->root_dir) != OK){
    rs = -IO_ERROR;
    goto end;
  }
  
end:
  if(rs < 0){
    free(private);
    disk->fs_private_data = 0;
  }
  if(stream){
    stream_free(stream);
  }
  return rs;
}

void *fat16_open(struct disk_t *disk, struct path_root *root, enum FILE_MODE mode){
  return 0;
}
