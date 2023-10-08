print_hex:
  ;numeric ascii values start at 0x30 and end at 0x39 (0..9). printhing hex strategy is converting the hex number to ascii char and 0x30 to result
  ;print_hex subroutine uses dx as parameter
  pusha
  mov cx, 0 ;for indexing
_hex_loop:
  cmp cx, 4 ;loop 4 times
  je _hex_end 
  mov ax, dx ;use ax as variable
  and ax, 0x000f ;masking for last byte
  add al, 0x30 ;
  ;convert last char of dx to ascii
  cmp al,0x39 ;if > 9 its not a digit but A..F value of hex 
  jle _hex_loop_2
  add al, 7 ;if hex digit is not a integer, add 7
_hex_loop_2:
  mov bx, _hex_out + 5 ;base + len
  sub bx, cx ;index
  mov [bx], al ;copy ascii char on al to position pointed by bx
  ror dx, 4 ;rotate right 4 times so
  inc cx
  jmp _hex_loop
_hex_end:
  mov bx ,_hex_out
  call print_str 
  popa
  ret
_hex_out: db '0x0000' , 0;reserved memory for new string
