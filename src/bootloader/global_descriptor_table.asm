
_gdt_start:
  dd 0x0 ;mandatory null descriptor of 8 byte
  dd 0x0 ;dd means double word which is 4 byte. by adding 2 of them null descriptor is complete

_gdt_code: ;code segment descriptor
  ;os_dev.pdf page 38
  ;base = 0x0 , limit = 0xffff
  ;1st flags -> present:1 privilege:00 descriptor_type:1 -> 1001b
  ;type flags -> code:1 conforming:0 readable:1 accessed:0 -> 1010b
  ;2nd flags -> granularity:1 32bit_default:1 64bit_segment:0 avl:0 -> 1100b
  dw 0xffff ;limit (bits 0-15)
  dw 0x0 ;base (bits 0-15)
  db 0x0 ;base (bits 16-23)
  db 10011010b ;1st fags ,type flags
  db 11001111b ;2nd flags, limit(bits 16-19)
  db 0x0 ;base (bits 24-31)

_gdt_data: ;data segment descriptor
  ;os_dev.pdf page 38
  ;same as code segment except for type flags
  ;type flags -> code:0 expand_down:0 writable:1 accessed:0 -> 0010b
  dw 0xffff ;limit (bits 0-15)
  dw 0x0 ;base (bits 0-15)
  db 0x0 ;base (bits 16-23)
  db 10010010b ;1st fags ,type flags
  db 11001111b ;2nd flags, limit(bits 16-19)
  db 0x0 ;base (bits 24-31)

_gdt_end:

gdt_descriptor: ;used to point cpu to the global descriptor table. stores gdt's start and length.
  ;stores gdt size (16 bit) , gdt address (32 bit)
  dw _gdt_end - _gdt_start - 1 ;size of gdt. always 1 less than true size (word, which is 2 byte(16bit))
  dd _gdt_start ;start address of gdt (double word, which is 4 byte(32bit))

  ;constants for segment offsets, segment registers must contain them when in protected mode
  CODE_SEG equ _gdt_code - _gdt_start
  DATA_SEG equ _gdt_data - _gdt_start
