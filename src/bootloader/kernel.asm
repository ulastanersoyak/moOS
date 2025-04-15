[bits 32]

global _start
extern kernel_main

_start:
	mov ax, 0x10; now in pm old segments are meaningless so change segments to the ones defined in gdt
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ebp, 0x00200000
	mov esp, ebp

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

times 512 - ($ - $$) db 0; align with c code
