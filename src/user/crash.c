#include "lib/moos.h"

// "crash" writes to kernel memory, "crash code" overwrites its own code. both
// page fault since kernel pages are supervisor only and code pages are read
// only, so the kernel kills the task
int
main (int argc, char **argv)
{
  if (argc > 1 && argv[1][0] == 'c')
    {
      print ("writing to my own code...\n");
      *(volatile uint8_t *)main = 0xc3;
      print ("still alive, code is writable!\n");
      return 1;
    }
  print ("writing to kernel memory at 0x100000...\n");
  *(volatile uint32_t *)0x100000 = 0xdeadbeef;
  print ("still alive, kernel memory is not protected!\n");
  return 1;
}
