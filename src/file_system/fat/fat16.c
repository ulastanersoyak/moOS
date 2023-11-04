#include "../file.h"
#include "fat16.h"
#include "../../libc/string/string.h"
#include "../../kernel/config.h"
#include "../../drivers/disk/disk_stream.h"

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
#define FAT16_FILE_RESERVED     0X40

// TODO: used fat32 as source. might be wrong
// FAT header re-implemented in c.
// base implementation is in bootloader.asm
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
  strcpy(fat16_fs.fs_name, "FAT16");
  return &fat16_fs;
}

int32_t fat16_resolve(struct disk_t *disk){
  
  return 0;
}

void *fat16_open(struct disk_t *disk, struct path_root *root, enum FILE_MODE mode){
  return 0;
}
