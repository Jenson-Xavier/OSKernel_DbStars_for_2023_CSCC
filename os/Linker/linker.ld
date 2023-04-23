OUTPUT_ARCH(riscv) 
ENTRY(kernel_entry)  

. = 0x80200000;

SECTIONS
{
    .text : {
        *(.text.entry) 
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    PROVIDE(etext = .); 

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    . = ALIGN(0x1000);

    .data : {
        *(.data)
        *(.data.*)
    }

    .sdata : {
        *(.sdata)
        *(.sdata.*)
    }

    .bss : {
        *(.bss)
        *(.bss.*)
        *(.sbss*)
    }

    PROVIDE(end = .);

    . = ALIGN(0x1000);

    PROVIDE(space_start = .);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}