section .text

global _start
extern main
extern exit

	;     program entry point, the kernel jumps here in ring 3 with argc and argv
	;     on top of the stack
_start:
	push  dword [esp+4]; argv
	push  dword [esp+4]; argc, shifted by the push above
	call  main
	push  eax
	call  exit
	jmp   $
