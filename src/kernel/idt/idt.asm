section .asm

global idt_load
global no_intr
global enable_intr
global disable_intr
global isr_stub_table
global isr128

extern interrupt_handler

enable_intr:
	sti
	ret

disable_intr:
	cli
	ret

idt_load:
	push ebp
	mov  ebp, esp

	mov  eax, [ebp+8]
	lidt [eax]

	pop ebp
	ret

	;   cpu pushes an error code for some exceptions. stubs without one push a
	;   dummy 0 so every handler sees the same frame layout

%macro ISR_NOERR 1
isr%1:
	push dword 0
	push dword %1
	jmp  isr_common
%endmacro

%macro ISR_ERR 1
isr%1:
	push dword %1
	jmp  isr_common
%endmacro

	;    cpu exceptions 0-31
	ISR_NOERR 0
	ISR_NOERR 1
	ISR_NOERR 2
	ISR_NOERR 3
	ISR_NOERR 4
	ISR_NOERR 5
	ISR_NOERR 6
	ISR_NOERR 7
	ISR_ERR   8
	ISR_NOERR 9
	ISR_ERR   10
	ISR_ERR   11
	ISR_ERR   12
	ISR_ERR   13
	ISR_ERR   14
	ISR_NOERR 15
	ISR_NOERR 16
	ISR_ERR   17
	ISR_NOERR 18
	ISR_NOERR 19
	ISR_NOERR 20
	ISR_ERR   21
	ISR_NOERR 22
	ISR_NOERR 23
	ISR_NOERR 24
	ISR_NOERR 25
	ISR_NOERR 26
	ISR_NOERR 27
	ISR_NOERR 28
	ISR_ERR   29
	ISR_ERR   30
	ISR_NOERR 31

	;    remapped pic irqs 0-15 (0x20-0x2f)
	%assign i 32
	%rep    16
	ISR_NOERR i
	%assign i i+1
	%endrep

	;         system call gate
	ISR_NOERR 128

isr_common:
	pushad
	;   save the interrupted data segment (user segments when coming from
	;   ring 3) and switch to kernel data segments
	mov ax, ds
	push eax
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp; pointer to struct interrupt_frame
	call interrupt_handler
	add  esp, 4

	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	popad
	add esp, 8; pop vector number and error code
	iret

	;   software interrupts without a handler
no_intr:
	iret

section .data

isr_stub_table:
	%assign i 0
	%rep    48
	dd      isr%+i
	%assign i i+1
	%endrep
