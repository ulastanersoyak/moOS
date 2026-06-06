#include <setjmp.h>

// setjmp saves the registers a cdecl function has to keep, the stack pointer
// of the caller and the return address. longjmp puts them back and returns
// val (1 for 0) from that setjmp a second time

__asm__ (".text\n"
         ".globl setjmp\n"
         "setjmp:\n"
         "  mov 4(%esp), %eax\n"
         "  mov %ebx, 0(%eax)\n"
         "  mov %esi, 4(%eax)\n"
         "  mov %edi, 8(%eax)\n"
         "  mov %ebp, 12(%eax)\n"
         "  lea 4(%esp), %ecx\n"
         "  mov %ecx, 16(%eax)\n"
         "  mov (%esp), %ecx\n"
         "  mov %ecx, 20(%eax)\n"
         "  xor %eax, %eax\n"
         "  ret\n"
         ".globl longjmp\n"
         "longjmp:\n"
         "  mov 4(%esp), %edx\n"
         "  mov 8(%esp), %eax\n"
         "  test %eax, %eax\n"
         "  jnz 1f\n"
         "  inc %eax\n"
         "1:\n"
         "  mov 0(%edx), %ebx\n"
         "  mov 4(%edx), %esi\n"
         "  mov 8(%edx), %edi\n"
         "  mov 12(%edx), %ebp\n"
         "  mov 16(%edx), %esp\n"
         "  jmp *20(%edx)\n");
