disk_load:
  push dx
  mov ah, 0x02 ;bios read sector function
  mov al, dh ;read dh sectors
  mov ch, 0x00 ;cylinder 0
  mov dh, 0x00 ;head 0
  mov cl, 0x02 ;start reading from second sector (after the boot sector)
  int 0x13 ;bios interrupt
  jc _disk_error ;interrupt failed if carry flag is set
  pop dx ;restore dx
  cmp dh, al ;al stores readed sectors count. if it is not equal to expected handle it
  jne _y
  ret
_y:
mov bx, _disk_error_msgg
jmp _x
_disk_error:
  mov bx, _disk_error_msg
_x:
  call print_str
  jmp $
_disk_error_msg: db 'disk read error 1' , 0
_disk_error_msgg: db 'disk read error 2' , 0
