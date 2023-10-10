[bits 32] ;using 32bit protected mode

video_memory equ 0xb8000
monochrome equ 0x0f
print_str_pm:
  ;print a string in protected mode. uses edx as parameter
  pusha
  mov eax, 160
  mul ecx ;cx will contain row number
  add eax, video_memory
  mov edx, eax

_str_pm_loop:
  mov al, [ebx] ;[ebx] is the address of the char
  mov ah, monochrome ;and high bits as metadata about char like background and foreground colours of char

  cmp al, 0 ;check if string is ended
  je _str_pm_return
  
  mov [edx], ax ;store char and attributes at current char cell
  add ebx, 1 ;increment ebx to next char on string
  add edx, 2 ;move to next char in video memory
  jmp _str_pm_loop

_str_pm_return:
  popa
  ret
