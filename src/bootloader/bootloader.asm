[org 0x7c00]

mov bp, 0x8000 ;set stack away from bootloader
mov sp, bp

mov bx, 0x9000 ;es:bx 0x0000:0x9000
mov dh, 2 ;read 2 sectors 

call disk_load_bios

mov dx, [0x9000]
call print_hex_bios

mov dx, [0x9000 + 512]
call print_hex_bios

jmp $

%include "./src/bootloader/print_str_bios.asm" ;includes print_str subroutin
%include "./src/bootloader/print_hex_bios.asm" ;includes print_hex subroutine
%include "./src/bootloader/disk_load_bios.asm" ;includes disk_load subroutine 

times 510 - ($-$$) db 0

dw 0XAA55

times 256 dw 0xdada
times 256 dw 0xface
