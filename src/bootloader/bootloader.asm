
[org 0x7c00]

mov bp, 0x9000 ;set the stack
mov sp, bp

mov bx, _msg_real_mode
call print_str_bios

call switch_to_pm ;program should never return from this, if it returns loop forever

jmp $ ;hopefully, this wont get executed

%include "./src/bootloader/bios_subroutines/print_str_bios.asm" ;includes print_str subroutin
;%include "./src/bootloader/bios_subroutines/print_hex_bios.asm" ;includes print_hex subroutine
;%include "./src/bootloader/bios_subroutines/disk_load_bios.asm" ;includes disk_load subroutine 
%include "./src/bootloader/pm_subroutines/global_descriptor_table.asm" ;includes gdt and gdt descriptor 
%include "./src/bootloader/pm_subroutines/print_str_pm.asm" ;includes print_str_pm subroutine for printing in 32bit pm
%include "./src/bootloader/pm_subroutines/switch_to_pm.asm" ;includes switch_to_pm subroutine for mode switch

[bits 32]
BEGIN_PM: ;where program arrives after switch_to_pm subroutine call
 mov ebx, _msg_protected_mode 
 call print_str_pm
 jmp $

_msg_real_mode: db "started in 16 bit real mode ", 0
_msg_protected_mode: db "landed in 32 bit pm ", 0

times 510 - ($-$$) db 0
dw 0XAA55
