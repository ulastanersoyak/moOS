disk_load_bios:
  ;reads sectors from disk. uses dx as parameter
  pusha
  push dx
  mov ah, 0x02 ;int 0x13 function -> 0x02 = read
  mov al, dh ;al -> number of sectors to read.
  mov cl, 0x02 ;cl -> sector (0x01 .. 0x11) 0x01 is boot sector, so 0x02 is first available sector
  mov ch, 0x00 ;ch -> cylinder
  mov dh, 0x00 ;dl -> head number

  int 0x13 ;bios interrupt

  jc _disk_error_bios ;interrupt failed if carry flag is set

  pop dx ;restore dx

  cmp al, dh;al stores readed sectors count. if it is not equal to expected handle it

  jne _read_error_bios
  
  popa
  ret

_disk_error_bios:
mov bx, _disk_error_msg_bios
call print_str_bios

mov dh, al ;ah = error code, dl = disk drive that dropped the error
call print_hex_bios

jmp _disk_error_loop_bios

_read_error_bios:
  mov bx, _disk_error_msg_bios
  call print_str_bios

_disk_error_loop_bios:
  jmp $

_disk_error_msg_bios: db 'disk read error ' , 0
_sectors_error_msg_bios: db 'incorect number of sectors read' , 0
