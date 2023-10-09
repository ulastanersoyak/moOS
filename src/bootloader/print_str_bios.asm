
print_str_bios:
  ;print_str subroutine that uses bx register as a parameter. bx will point to the start of string that will be printed
  ;while string[i] != 0 -> print string[i] and i++, else return from subroutine. preserves register values.
  pusha ;pushes all register to stack to prevent any changes
  mov ah, 0x0e ;0x10 interrupt prints when a registers high 4 bit is 0x0e 
_str_bios_loop:
  mov al, [bx] ;the ascii char goes to a registers lowest 4 bit
  cmp al, 0 ;compare char with 0. if it is 0 equal flag bit will be set
  je _str_bios_return ;jump equal (je) jumps if equal flag is set
  int 0x10 ;al has the char and ah has the 0x0e so int 0x10 will print the char to the string
  inc bx ;incrementing bx by one will set address to next potential char
  jmp _str_bios_loop ;loop over
_str_bios_return:
  popa ;pop all registers to restore them to their state 
  ret ;return from subroutine
