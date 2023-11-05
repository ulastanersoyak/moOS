org 0x7c00

bits 16

CODE_SEG equ _gdt_code - _gdt_start
DATA_SEG equ _gdt_data - _gdt_start

jmp short boot_start2
nop

;fat16 header
OEMIdentifier     db 'MOOS    ' ;8 byte oemidentifier
BytesPerSector    dw 0x200 ;512 bytes per sector
SectorsPerCluster db 0x80
ReservedSectors   dw 200 ;reserved sectors for kernel
FATCopies         db 0x02
RootDirEntries    dw 0x40
NumSectors        dw 0x00
MediaType         db 0xF8
SectorsPerFAT     dw 0x100
SectorsPerTrack   dw 0x20
NumberOfHeads     dw 0x40
HiddenSectors     dd 0x00
SectorsBig        dd 0x773594
DriveNumber       db 0x80;extended bpb
WinNTBit          db 0x00
Signature         db 0x29
VolumeID          dd 0xD105
VolumeIDString    db 'VOLUMEIDSTR' ;11 byte
SystemIDString    db 'SYSIDSTR';8 byte




boot_start2:
  jmp 0:step2

step2:
  ; set segments to zero to fil them up later
  xor ax, ax
  cli
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, 0x7c00
  sti

switch_to_pm: ;switch to protected mode

  cli ;clear interrupt. cpu will ignore any future interrupts that may interrupt mode switching.interrupt handling is different from real mode on 
  ;protected mode. current interrupt vector table set by bios which will run 16 bit code, which cant handle 32 bit segments defined in gdt
  lgdt [gdt_descriptor] ;load gdt instruction. which will take address of descriptor
  ;making actual switch by setting the first bit of a special cpu control register called cr0.
  mov eax, cr0
  or eax, 0x1
  mov cr0, eax ;after this instruction cpu is in 32 bit protected mode

  jmp CODE_SEG:load_kernel ;making a long jump to CODE_SEG label. which will force cpu to flush any pre fetched and decoded instructions that may
                                    ;be in 16 bit to prevent future crashes of system and since current code segment is not valid in protected mode,
                                    ;this instruction sets code segmenttoo. (os_dev.pdf page 42)
;set gdt for pm
_gdt_start:

_gdt_null:
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

bits 32
load_kernel:
  mov eax, 1 ;start from sector 1 cuz sector 0 is the bootloader itself
  mov ecx, 100 ;makefile appends 100 sector worth of space for kernel so read 100 sectors. if number is less kernel cant be fully loaded
              ;if number is higher ud
  mov edi, 0x0100000 ;address to load
  call ata_lba_read
  jmp CODE_SEG:0x0100000 ;jump to kernel

ata_lba_read:
  mov ebx, eax ;backup lba
  shr eax, 24 ;get 8 highest bits of lba
  or eax, 0xe0 ;select master drive
  mov dx, 0x1f6 ;port
  out dx, al ; send 8 bits to lba
  
  ;send total sectors to read
  ; written in c in disk.c
  mov eax, ecx 
  mov dx, 0x1f2
  out dx, al

  mov eax, ebx
  mov dx, 0x1f3
  out dx, al  

  mov dx, 0x1f4
  mov eax, ebx
  shr eax, 8
  out dx, al
  
  mov dx, 0x1f5
  mov eax, ebx
  shr eax, 16
  out dx, al

  mov dx, 0x1f7
  mov al, 0x20
  out dx, al

;read all sectors into memory
_next_sector:
  push ecx

res:
  mov dx, 0x1f7
  in al, dx
  test al, 8 
  jz res

  ;read 256 words at a time
  mov ecx, 256
  mov dx, 0x1f0
  rep insw ;reads a word 256 times. each word is 2 byte. thus making a total of 512 bytes (1 sector) at a time
  pop ecx
  loop _next_sector
  ret

times 510 - ($-$$) db 0
dw 0xaa55
