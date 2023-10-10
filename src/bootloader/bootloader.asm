[org 0x7c00]

kernel_offset equ 0x1000

mov [boot_drive], dl ;bios sets the boot drive in dl on boot

mov bp, 0x9000 ;set the stack
mov sp, bp

;start boot process
mov bx, _msg_real_mode
call print_str_bios
call print_nl

call load_kernel ;read kernel from disk
call switch_to_pm ;program should never return from this, if it returns loop forever

jmp $ ;hopefully, this wont get executed

%include "./src/bootloader/print_str_bios.asm" ;includes print_str_bios subroutine
%include "./src/bootloader/print_new_line_bios.asm" ;includes print_nl subroutine
%include "./src/bootloader/print_hex_bios.asm" ;includes print_hex_bios subroutine
%include "./src/bootloader/disk_load_bios.asm" ;includes disk_load_bios subroutine 
%include "./src/bootloader/global_descriptor_table.asm" ;includes gdt and gdt descriptor 
%include "./src/bootloader/print_str_pm.asm" ;includes print_str_pm subroutine for printing in 32bit pm
%include "./src/bootloader/switch_to_pm.asm" ;includes switch_to_pm subroutine for mode switch

[bits 16]
load_kernel:
  mov bx,_msg_load_kernel
  mov cx, 13
  call print_str_bios

  mov bx, kernel_offset ;read from disk and store in 0x1000
  mov dh, 2
  mov dl, [boot_drive]
  call disk_load_bios

  mov bx, _ok
  call print_str_bios
  call print_nl
  ret

[bits 32]

BEGIN_PM: ;where program arrives after switch_to_pm subroutine call

  mov ebx, _msg_protected_mode 
  call print_str_pm

  mov ebx, _msg_giving_kernel
  mov cx, 14
  call print_str_pm

  call kernel_offset ;give control to kernel
  jmp $ ;ideally would never return from kernel call. but loops forever if kernel ever returns (hope it doesnt :< )

boot_drive db 0
_ok: db " [OK]", 0
_msg_real_mode: db "[b] booting started...", 0
_msg_load_kernel: db "[b] loading kernel into memory", 0
_msg_protected_mode: db "[b] switched to protected mode [OK]", 0
_msg_giving_kernel: db "[b] giving controll to kernel [OK]", 0

times 510 - ($-$$) db 0
dw 0XAA55
