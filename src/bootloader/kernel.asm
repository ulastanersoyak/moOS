[bits 32]

	;   kernel entry for both bootloaders. the moOS bootloader loads the flat
	;   kernel.bin to 0x100000 and jumps to its first byte (_start). multiboot
	;   loaders like limine load kernel.elf and jump to its elf entry point
	;   (multiboot_entry) with eax = MULTIBOOT_BOOTLOADER_MAGIC and ebx pointing
	;   to the multiboot info structure

global _start
global multiboot_entry
extern kernel_main
extern __bss_start
extern __bss_end

MULTIBOOT_HEADER_MAGIC equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS equ 0x00000000; elf kernel, no extra requests

KERNEL_CODE_SEG equ 0x08
KERNEL_DATA_SEG equ 0x10
KERNEL_STACK    equ 0x00200000

	;       placed first in the kernel image by the linker script
	section .boot

_start:
	jmp own_bootloader_entry

	;     must be 4 byte aligned and within the first 8 kb of kernel.elf
	align 4

multiboot_header:
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

own_bootloader_entry:
	;   the moOS bootloader already loaded a flat gdt and passes no boot info
	xor eax, eax
	xor ebx, ebx
	jmp common_entry

multiboot_entry:
	;    the multiboot spec leaves the gdt undefined, load one before touching
	;    segment registers. kernel_main replaces it with the gdt built in c
	lgdt [boot_gdt_descriptor]
	jmp  KERNEL_CODE_SEG:common_entry

common_entry:
	mov cx, KERNEL_DATA_SEG
	mov ds, cx
	mov ss, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	mov ebp, KERNEL_STACK
	mov esp, ebp

	;    zero the bss. multiboot loaders do it already, but the flat kernel.bin
	;    doesnt contain the bss at all
	mov  edx, eax
	cld
	mov  edi, __bss_start
	mov  ecx, __bss_end
	sub  ecx, edi
	xor  eax, eax
	rep  stosb
	mov  eax, edx

	;    kernel_main (boot magic, boot info)
	push ebx
	push eax

	;   remap the programmable interrupt controllers so irqs dont collide with
	;   cpu exceptions. master irqs -> 0x20-0x27, slave irqs -> 0x28-0x2f
	mov al, 00010001b; icw1: init, cascade mode, icw4 needed
	out 0x20, al
	out 0xA0, al

	mov al, 0x20; icw2: master vector offset
	out 0x21, al
	mov al, 0x28; icw2: slave vector offset
	out 0xA1, al

	mov al, 00000100b; icw3: slave is on master irq2
	out 0x21, al
	mov al, 00000010b; icw3: slave cascade identity
	out 0xA1, al

	mov al, 00000001b; icw4: 8086 mode
	out 0x21, al
	out 0xA1, al

	;   mask every irq except timer (irq0), keyboard (irq1) and the slave
	;   cascade (irq2)
	mov al, 11111000b
	out 0x21, al
	mov al, 11111111b
	out 0xA1, al

	call kernel_main

	jmp $

	;     flat 4 gb code and data segments, same as the moOS bootloader gdt
	align 8

boot_gdt:
	dq 0x0000000000000000; null
	dq 0x00CF9A000000FFFF; code: base 0, limit 4 gb, ring 0, 32 bit
	dq 0x00CF92000000FFFF; data: base 0, limit 4 gb, ring 0, 32 bit

boot_gdt_descriptor:
	dw $ - boot_gdt - 1
	dd boot_gdt
