[bits 32]

global _start
extern kernel_main

_start:
  mov ax, 0x10 ;now in pm old segments are meaningless so change segments to the ones defined in gdt
  mov ds, ax
  mov ss, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ebp, 0x00200000
  mov esp, ebp

  ;enable a20 line
  in al, 0x92
  or al, 2 
  out 0x92, al

  ;remap the master programmable interrupt controller
  mov al, 00010001b
  out 0x20, al

  mov al, 0x20 ;master pic. number is completely random
  out 0x21, al

  mov al, 00000001b
  out 0x21, al
  ;remap master pic

  sti

  call kernel_main

  jmp $

times 512 - ($ - $$) db 0 ;allign with c code
