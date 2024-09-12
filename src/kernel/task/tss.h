#ifndef TSS_H
#define TSS_H

#include <stdint.h>

// clang-format off
// task switch segment.
struct tss
{
  uint32_t link;
  uint32_t esp0; // kernel stack ptr
  uint32_t ss0;  // kernel stack segment
  uint32_t esp1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t sr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldtr;
  uint32_t iopb;
}__attribute__ ((packed));
// clang-format on

void tss_load (uint32_t tss_segment);

#endif // !TSS_H
