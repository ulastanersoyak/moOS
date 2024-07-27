#include "disk.h"
#include "../../kernel/io/io.h"
#include "../../kernel/config.h"
#include "../../libc/string/string.h"
#include "../../libc/stdio/stdio.h"
#include "../screen/terminal.h"

#include <stddef.h>

struct disk_t main_master_disk;
//TODO: ADD ALL DISK TYPES
int32_t disk_read(int32_t logical_block_addr, uint32_t total_block, void *buffer){
  // works same as bootloader disk read
  outb(0x1F6, (logical_block_addr >> 24) | 0xE0);
  outb(0x1F2, total_block);
  outb(0x1F3, (uint16_t)(logical_block_addr & 0xff));
  outb(0x1F4, (uint16_t)(logical_block_addr >> 8));
  outb(0x1F5, (uint16_t)(logical_block_addr >> 16));
  outb(0x1F7, 0x20);  
  uint16_t *buff = (uint16_t*)buffer;
  for(size_t i = 0; i < total_block ; i++){
    char c = insb(0x1F7);
    while(!(c & 0x08)){
      // wait until 0x08th bit is set
      c = insb(0x1F7);
    }
    // read 256 times since insw reads a word which is 2 byte.
    // 256x2 = 512 byte (1 sector)
    for(size_t j = 0; j < 256 ; j++){
      *buff = insw(0x1f0);
      buff++;
    }
  }  
  return OK;
}

//just works on disk 0 (main mastar disk)
// TODO: maybe add more disks
struct disk_t* get_disk(uint32_t idx){
  if(idx != 0){
    return 0;
  }
  return &main_master_disk;
}

void init_main_master_disk(uint8_t verbose){
  memset(&main_master_disk, 0, sizeof(main_master_disk));
  main_master_disk.type = REAL_DISK_TYPE;
  main_master_disk.sector_size = MASTER_MAIN_DISK_SECTOR_SIZE; 
  main_master_disk.id = 0;
  main_master_disk.file_system = fs_resolve(&main_master_disk);
  if(verbose){
    printf("master disk driver init");
    if(!main_master_disk.file_system){
      init_ER();
    }else{
      printf(" <FS: %s>",main_master_disk.file_system->fs_name);
      init_OK();
    }
  }
}

int32_t disk_read_block(struct disk_t *disk, uint32_t logical_block_addr, uint32_t total_block, void *buffer){
  if(disk != &main_master_disk){
    return -IO_ERROR;
  }
  return disk_read(logical_block_addr,total_block,buffer);
}
