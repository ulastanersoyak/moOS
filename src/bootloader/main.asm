[org 0x7c00]
mov bx, himsg
call print_str

mov bx, goodbye
call print_str

mov dx, 0xdead
call print_hex

jmp $

%include "./src/bootloader/print_str.asm" ;includes print_str subroutine to safely printing ascii chars to the screen uses bx as parameter holder
%include "./src/bootloader/print_hex.asm" ;includes print_hex subroutine to safely printing hex digits to the screen uses dx as parameter holder

himsg: db 'erm hello mate ', 0
goodbye: db 'i guess it works '

times 510 - ($-$$) db 0

dw 0XAA55
