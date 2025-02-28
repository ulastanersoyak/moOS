ENTRY(_start)
OUTPUT_FORMAT(binary)
SECTIONS
{
    . = 1M;
    .text : ALIGN(4096)
    {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4096)
    {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    .asm : ALIGN(4096)
    {
      *(.asm)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}
