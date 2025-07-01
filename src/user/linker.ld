ENTRY(_start)

/* one load segment per permission. the loader maps code and rodata pages
   read only, so programs cant overwrite their own code */
PHDRS
{
    text PT_LOAD FLAGS(5);   /* r-x */
    rodata PT_LOAD FLAGS(4); /* r-- */
    data PT_LOAD FLAGS(6);   /* rw- */
}

SECTIONS
{
    /* must match USER_CODE_ADDR in src/kernel/config.h */
    . = 0x40000000;
    .text :
    {
        *(.text)
        *(.text.*)
    } :text

    . = ALIGN(4096);
    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    } :rodata

    . = ALIGN(4096);
    .data :
    {
        *(.data)
        *(.data.*)
    } :data

    /* not stored in the file, the loader zeroes it */
    .bss :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    } :data

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}
