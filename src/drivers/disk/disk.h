#ifndef DISK_H
#define DISK_H

#include <stdint.h> 

// starting from logical_block_addr, reads total_block amount of blocks into the given buffer
int32_t disk_read(int32_t logical_block_addr, int total_block, void *buffer);

#endif // !DISK_H
