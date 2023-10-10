print_nl:
  pusha
  mov ah, 0x0e
  mov al, 0x0a ;new line character
  int 0x10
  mov al, 0x0d ;carriage return
  int 0x10
  popa
  ret
