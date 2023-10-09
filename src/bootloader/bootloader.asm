[org 0x7c00]

cli ;clear interrupt. cpu will ignore any future interrupts that may interrupt mode switching.interrupt handling is different from real mode on protected     ;mode. current interrupt vector table set by bios which will run 16 bit code, which cant handle 32 bit segments defined in gdt
lgdt [gdt_descriptor] ;load gdt instruction. which will take address of descriptor
;making actual switch by setting the first bit of a special cpu control register called cr0.


jmp $

%include "./src/bootloader/bios_subroutines/print_str_bios.asm" ;includes print_str subroutin
%include "./src/bootloader/bios_subroutines/print_hex_bios.asm" ;includes print_hex subroutine
%include "./src/bootloader/bios_subroutines/disk_load_bios.asm" ;includes disk_load subroutine 
%include "./src/bootloader/global_descriptor_table.asm" ;includes gdt and gdt descriptor for switching to 32 bit protected mode from 16 bit real mode
times 510 - ($-$$) db 0

dw 0XAA55
