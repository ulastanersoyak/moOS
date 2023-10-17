section .asm

global idt_load
global int21h
global no_intr

extern int21h_handler
extern no_intr_handler

idt_load:
  push ebp
  mov ebp,esp

  mov ebx, [ebp+8]
  lidt [ebx]

  pop ebp
  ret

int21h:
  ;keyboard interrupt
  cli
  pushad
  call int21h_handler
  popad
  sti
  iret

no_intr:
  cli
  pushad
  call no_intr_handler 
  popad
  sti
  iret
