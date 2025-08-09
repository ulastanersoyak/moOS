/* entry point used by multiboot loaders. the moOS bootloader ignores it and
   jumps to the start of the flat image instead, where _start sits */
ENTRY(multiboot_entry)
SECTIONS
{
    . = 1M;
    .text : ALIGN(4096)
    {
        /* entry stub and multiboot header first */
        *(.boot)
        *(.text)
        *(.text.*)
        *(.asm)
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

    /* zeroed by kernel.asm, not stored in the flat kernel.bin */
    .bss : ALIGN(4096)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}
