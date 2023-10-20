[bits 16]

switch_to_pm: ;switch to protected mode

  cli ;clear interrupt. cpu will ignore any future interrupts that may interrupt mode switching.interrupt handling is different from real mode on 
  ;protected mode. current interrupt vector table set by bios which will run 16 bit code, which cant handle 32 bit segments defined in gdt
  lgdt [gdt_descriptor] ;load gdt instruction. which will take address of descriptor
  ;making actual switch by setting the first bit of a special cpu control register called cr0.
  mov eax, cr0
  or eax, 0x1
  mov cr0, eax ;after this instruction cpu is in 32 bit protected mode

  jmp CODE_SEG:init_pm ;making a long jump to CODE_SEG label. which will force cpu to flush any pre fetched and decoded instructions that may
                                    ;be in 16 bit to prevent future crashes of system and since current code segment is not valid in protected mode,
                                    ;this instruction sets code segmenttoo. (os_dev.pdf page 42)
[bits 32]
init_pm:
  ;by now cpu is guarenteed to be in 32 bit protected mode
  mov ax, DATA_SEG ;now in pm old segments are meaningless so change segments to the ones defined in gdt
  mov ds, ax
  mov ss, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  mov ebp, 0x90000 ;update stack position so its top of the free space
  mov esp, ebp

  call BEGIN_PM
