[org 0x7c00]

mov dx, 0xdead
call print_hex

mov bx, himsg
call print_str

mov bx, goodbye
call print_str

mov [boot_drive], dl ;bios stores boot drive in dl so store it

mov bp, 0x8000 ;set stack away from bootloader
mov sp, bp

mov bx, 0x9000 ;load 5 sectors to 0x0000(es):0x9000(bx)
mov dh, 5
mov dl, [boot_drive]
call disk_load 

mov dx,[0x9000]
call print_hex

mov dx,[0x9000+512]
call print_hex

jmp $

%include "./src/bootloader/print_str.asm" ;includes print_str subroutin
%include "./src/bootloader/print_hex.asm" ;includes print_hex subroutine
%include "./src/bootloader/disk_load.asm" ;includes disk_load subroutine 

himsg: db ' erm hello mate ', 0
goodbye: db 'i guess it works '
boot_drive: db 0

times 510 - ($-$$) db 0

dw 0XAA55

times 256 dw 0xdada
times 256 dw 0xface
