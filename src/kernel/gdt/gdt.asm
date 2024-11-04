section .asm

global gdt_raw_load

KERNEL_CODE_SEG equ 0x08
KERNEL_DATA_SEG equ 0x10

gdt_raw_load:
	mov  eax, [esp+4]
	mov  [gdt_descriptor + 2], eax
	mov  ax, [esp+8]
	mov  [gdt_descriptor], ax
	lgdt [gdt_descriptor]
	;    segment registers cache the old descriptors until they are reloaded
	jmp  KERNEL_CODE_SEG:.reload_cs

.reload_cs:
	mov ax, KERNEL_DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	ret

section .data

gdt_descriptor:
	dw 0x00; size
	dd 0x00; gdt start address
