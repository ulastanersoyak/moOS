section .asm

global fpu_enable
global fpu_save
global fpu_restore

	;   void fpu_enable (void)
	;   lets ring 0 and ring 3 use the x87 fpu: clears cr0.EM (emulation)
	;   and cr0.TS (task switched) so fpu instructions dont fault, sets
	;   cr0.MP and cr0.NE so wait and fpu errors behave like on a 486 and up
fpu_enable:
	mov   eax, cr0
	and   eax, ~((1 << 2) | (1 << 3))
	or    eax, (1 << 1) | (1 << 5)
	mov   cr0, eax
	fninit
	ret

	;   void fpu_save (void *state)
	;   stores the 108 byte fpu state and resets the fpu, like fninit
fpu_save:
	mov   eax, [esp+4]
	fnsave [eax]
	ret

	;   void fpu_restore (const void *state)
fpu_restore:
	mov   eax, [esp+4]
	frstor [eax]
	ret
