#ifndef _SETJMP_H
#define _SETJMP_H

// ebx, esi, edi, ebp, esp and the return address
typedef unsigned long jmp_buf[6];

__attribute__ ((returns_twice)) int setjmp (jmp_buf env);
__attribute__ ((noreturn)) void longjmp (jmp_buf env, int val);

#endif
