#ifndef CONFIG_H
#define CONFIG_H

#define TOTAL_INTERRUPTS 256

#define KERNEL_CODE_SEG 0x08 // start of code segment determined in gdt

#define KERNEL_DATA_SEG 0x10 // start of data segment determined in gdt

// heap constants
#define HEAP_SIZE_BYTE 104857600 /* 100 mb default heap*/

#define HEAP_BLOCK_SIZE 4096 // 4 kb block size

#define HEAP_ADDRESS 0x01000000 // address of heap start

#define HEAP_TABLE_ADDRESS 0x00007E00

// error message codes
#define OK 0

#define IO_ERROR 1

#define NO_MEMORY_ERROR 2

#define INVALID_ARG_ERROR 3

#endif // !CONFIG_H
