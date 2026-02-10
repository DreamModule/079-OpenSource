/* Linker script for SCP-079 kernel */

ENTRY(_start)

SECTIONS
{
    /* Kernel is loaded at 64KB physical address (0x10000).
     * BSS must end below 0xA0000 (VGA memory hole). */
    . = 0x10000;

    .text ALIGN(4K) : {
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
