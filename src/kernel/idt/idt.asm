section .asm

global idt_load
global int21h
global no_intr
global enable_intr
global disable_intr

extern int21h_handler
extern no_intr_handler

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

int21h:
	;keyboard interrupt

	cli
	pushad
	call int21h_handler
	popad
	sti
	iret

no_intr:
	cli
	pushad
	call no_intr_handler
	popad
	sti
	iret
