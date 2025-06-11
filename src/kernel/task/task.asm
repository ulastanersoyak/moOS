section .asm

global context_switch
global task_user_start
global task_kernel_start
global irq_save
global irq_restore

USER_DATA_SEG equ 0x23

	;   void context_switch (uint32_t *old_esp, uint32_t new_esp)
	;   saves callee saved registers on the current kernel stack, stores the
	;   stack pointer in old_esp and resumes the task whose stack is new_esp
context_switch:
	mov  eax, [esp+4]
	mov  edx, [esp+8]
	push ebp
	push ebx
	push esi
	push edi
	mov  [eax], esp
	mov  esp, edx
	pop  edi
	pop  esi
	pop  ebx
	pop  ebp
	ret

	;   first context_switch into a new user task returns here. the kernel
	;   stack holds an iret frame (eip, cs, eflags, esp, ss) for ring 3
task_user_start:
	mov ax, USER_DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	iret

	;    first context_switch into a new kernel thread returns here. ebx holds
	;    the thread function
task_kernel_start:
	sti
	call ebx
	jmp  $; kernel threads never return

	;   uint32_t irq_save (void)
	;   disables interrupts and returns the previous eflags
irq_save:
	pushfd
	pop eax
	cli
	ret

	;   void irq_restore (uint32_t eflags)
irq_restore:
	push dword [esp+4]
	popfd
	ret
