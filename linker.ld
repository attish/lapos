ENTRY (loader)

SECTIONS
{
    . = 0x00100000;

    .text ALIGN (0x1000) :
    {
        *(.text)
    }

    .rodata ALIGN (0x1000) :
    {
        *(.rodata*)
    }

    .data ALIGN (0x1000) :
    {
        *(.data)
    }

    .memblock ALIGN (0x1000) :
    {
        *(.memblock)
    }

    .bss :
    {
        sbss = .;
        *(COMMON)
        *(.bss)
        ebss = .;
    }

}
