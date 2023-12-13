bits 32

section .asm

global load_page_dir
global enable_paging_asm

load_page_dir:
  push ebp
  mov ebp,esp
  mov eax, [ebp+8]

  ;at this point eax holds paging directory pointer 
  mov cr3, eax

  pop ebp
  ret

enable_paging_asm:
  push ebp
  mov ebp,esp

  ;cant change cr0 directly
  mov eax, cr0
  or eax, 0x80000000
  mov eax,cr0

  pop ebp
  ret
