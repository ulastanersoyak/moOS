bits 32

section .asm

global load_page_dir
global enable_paging

load_page_dir:
  push ebp
  mov ebp, esp
  mov eax, [ebp+8]
  ; at this point eax holds systems paging directory pointer 
  mov cr3, eax
  pop ebp
  ret

enable_paging:
  push ebp
  mov ebp, esp

  ; cant change cr0 directly
  mov eax, cr0
  ; bit 31 enables paging
  or eax, 0x80000000
  mov cr0, eax

  pop ebp
  ret