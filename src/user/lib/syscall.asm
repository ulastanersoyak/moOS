section .text

	;   system call wrappers. number goes in eax, arguments in ebx, ecx, edx.
	;   numbers must match src/kernel/syscall/syscall.h

	;    $ marks the names as identifiers, wait is also an x86 instruction
%macro SYSCALL 2
global $%1
$%1:
	push ebx; callee saved in cdecl
	mov  eax, %2
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	mov  edx, [esp+16]
	int  0x80
	pop  ebx
	ret
%endmacro

	;    names that mean something else in standard c are weak, the c library
	;    in src/user/libc replaces them. moos_<name> always is the system call
%macro WEAK_SYSCALL 2
global $%1:weak
global moos_%1
$%1:
moos_%1:
	push ebx
	mov  eax, %2
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	mov  edx, [esp+16]
	int  0x80
	pop  ebx
	ret
%endmacro

	SYSCALL exit, 0
	SYSCALL print, 1
	SYSCALL putchar, 2
	SYSCALL getchar, 3
	SYSCALL sleep, 4
	SYSCALL uptime, 5
	SYSCALL sleep_until, 6
	SYSCALL yield, 7
	SYSCALL getpid, 8
	WEAK_SYSCALL open, 9
	SYSCALL read, 10
	SYSCALL write, 11
	SYSCALL close, 12
	SYSCALL filesize, 13
	SYSCALL listdir, 14
	SYSCALL sbrk, 15
	SYSCALL remove, 16
	SYSCALL mkdir, 17
	SYSCALL seek, 18
	SYSCALL pipe, 19
	SYSCALL spawn, 20
	SYSCALL wait, 21
	SYSCALL isatty, 22
	WEAK_SYSCALL time, 23
	SYSCALL chdir, 24
	WEAK_SYSCALL getcwd, 25
	SYSCALL tasks, 26
	SYSCALL kill, 27
	SYSCALL sched, 28
	SYSCALL setprio, 29
	SYSCALL run, 30
	SYSCALL clear, 31
	SYSCALL setcolor, 32
	SYSCALL getcursor, 33
	SYSCALL setcursor, 34
	SYSCALL sysinfo, 35
	SYSCALL reboot, 36
	SYSCALL mutex_open, 37
	SYSCALL mutex_lock, 38
	SYSCALL mutex_unlock, 39
	SYSCALL mutex_close, 40
	SYSCALL period, 41
	SYSCALL wait_period, 42
	SYSCALL gfx_mode, 43
	SYSCALL gfx_palette, 44
	SYSCALL gfx_blit, 45
	SYSCALL key_event, 46
	SYSCALL mouse_event, 47
	SYSCALL ignore_ctrl_c, 48
	SYSCALL pending, 49
