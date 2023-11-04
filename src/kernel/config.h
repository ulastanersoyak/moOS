#ifndef CONFIG_H
#define CONFIG_H

#define TOTAL_INTERRUPTS 256

#define KERNEL_CODE_SEG 0x08 // start of code segment determined in gdt

#define KERNEL_DATA_SEG 0x10 // start of data segment determined in gdt

// heap constants
#define HEAP_SIZE_BYTE 104857600 // 100 mb default heap

#define HEAP_BLOCK_SIZE 4096 // 4 kb block size

#define HEAP_ADDRESS 0x01000000 // heap memory start location

#define HEAP_TABLE_ADDRESS 0x00007E00 // heap table memory location

// paging constants
#define TOTAL_ENTRIES_PER_TABLE 1024

#define TOTAL_ENTRIES_PER_DIRECTORY 1024

#define PAGE_SIZE 4096

// disk constants
#define MASTER_MAIN_DISK_SECTOR_SIZE 512

// error message codes
#define OK 0

#define IO_ERROR 1

#define NO_MEMORY_ERROR 2

#define INVALID_ARG_ERROR 3

#define INVALID_PATH_ERROR 4

#define FILE_HANDLER_OVERFLOW 5

// printf max format string char
#define MAX_STRING_FORMAT_CHAR 100

#define RAND_MAX 32767 // 2^15 -1

// file system consts

#define MAX_PATH_LEN 100

#define MAX_FILE_SYSTEMS 12

// max open files
#define MAX_FILE_DESCS 512

#endif // !CONFIG_H
