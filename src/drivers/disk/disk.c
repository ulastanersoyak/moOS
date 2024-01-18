#include "disk.h"
#include "../../kernel/io/io.h"
#include "../../kernel/config.h"

#include <stddef.h>

int32_t disk_read(int32_t logical_block_addr, int total_block, void *buffer){
  // works same as bootloader disk read
  outb(0x1F6, (logical_block_addr >> 24) | 0xE0);
  outb(0x1F2, total_block);
  outb(0x1F3, (unsigned char)(logical_block_addr & 0xff));
  outb(0x1F4, (unsigned char)(logical_block_addr >> 8));
  outb(0x1F5, (unsigned char)(logical_block_addr >> 16));
  outb(0x1F7, 0x20);  
  unsigned short *buff = (unsigned short*)buffer;
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
