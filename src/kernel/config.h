#ifndef CONFIG_H
#define CONFIG_H

#define ERROR(val) (void *)(val)
#define ERROR_I(val) (int)(val)
#define IS_ERR(val) ((int)(val) < 0)

#define TOTAL_INTERRUPTS 256

// 32 cpu exceptions + 16 pic irqs have asm stubs
#define TOTAL_ISR_STUBS 48

// pic ports and commands. pics are remapped to 0x20-0x2f in kernel.asm
#define PIC_MASTER_COMMAND 0x20
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_EOI 0x20
#define PIC_MASTER_OFFSET 0x20

#define KERNEL_CODE_SEG 0x08 // start of code segment determined in gdt

#define KERNEL_DATA_SEG 0x10 // start of data segment determined in gdt

#define TSS_SEG 0x28 // tss selector, 6th entry of gdt

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

#define INVALID_DISK_ERROR 6

#define SIGNATURE_ERROR 7

#define NON_PERMITTED_READ_ERROR 8

#define UNIMPLEMENTED_FEATURE_ERROR 9

#define NOT_DIRECTORY_ERROR 10

#define NOT_FOUND_ERROR 11

#define DIR_NOT_EMPTY_ERROR 12

#define FILE_EXISTS_ERROR 13

#define BROKEN_PIPE_ERROR 14

// printf max format string char
#define MAX_STRING_FORMAT_CHAR 100

#define RAND_MAX 32767 // 2^15 -1

// file system consts

#define MAX_PATH_LEN 108

#define MAX_FILE_SYSTEMS 12

// max open files
#define MAX_FILE_DESCS 512

#define TOTAL_GDT_SEGMENTS 6

// mirror terminal output to qemu's debug console port (qemu -debugcon)
#define DEBUG_CONSOLE 1

#define DEBUG_CONSOLE_PORT 0xE9

// mirror terminal output to com1
#define SERIAL_CONSOLE 1

#define PIT_FREQUENCY_HZ 100

// user tasks. programs are elf executables linked at USER_CODE_ADDR. the user
// address space is USER_SPACE_TABLES page tables (4mb each) from there, a
// table is only allocated once a page in it is used
#define USER_CODE_SEG 0x1B // gdt entry 3 with rpl 3
#define USER_DATA_SEG 0x23 // gdt entry 4 with rpl 3
#define USER_CODE_ADDR 0x40000000
#define USER_STACK_SIZE (16 * 1024)
#define USER_SPACE_TABLES 16
#define USER_SPACE_SIZE (USER_SPACE_TABLES * TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE)
#define USER_STACK_TOP (USER_CODE_ADDR + USER_SPACE_SIZE)

// tasks
#define MAX_TASKS 16
#define TASK_KERNEL_STACK_SIZE (16 * 1024)
#define TASK_NAME_LEN 16
// round robin time slice
#define SCHED_QUANTUM_TICKS 5
// real time priorities, higher runs first. idle always has the lowest. the
// shell sits above every user task so a runaway high priority program cant
// lock out the console, killed tasks get the max so they can exit
#define TASK_PRIORITY_MAX 31
#define TASK_PRIORITY_SHELL 30
#define TASK_PRIORITY_USER_MAX 29
#define TASK_PRIORITY_DEFAULT 10
// file descriptors of a task: 0 stdin, 1 stdout, 2 stderr, then open files
// and pipes
#define TASK_MAX_FDS 16
// programs typed without a path are looked up here
#define TASK_BIN_DIR "0:/bin/"
#define TASK_BIN_EXT ".elf"
// started by the init thread at boot and again whenever it exits
#define TASK_SHELL_PATH "0:/bin/sh.elf"
// program arguments copied to the top of the user stack
#define TASK_MAX_ARGS 16
#define TASK_MAX_ARGS_BYTES 1024
// user heap grows from the end of the program up to the stack guard page
#define USER_HEAP_LIMIT (USER_STACK_TOP - USER_STACK_SIZE - PAGE_SIZE)

#define SYSCALL_INTERRUPT 0x80

#endif // !CONFIG_H
