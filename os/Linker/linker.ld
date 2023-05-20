OUTPUT_ARCH(riscv) 
ENTRY(kernel_entry)  

/*  . = 0x80200000;   */
. = 0xFFFFFFFF80200000;   /* 启动虚存机制后链接脚本的地址也需要相应的更改 */

SECTIONS
{
    /* 这里需要增加AT 装载到正确的物理地址 */
    .text : AT(0x80200000) {
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

    PROVIDE(kernel_end = .);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}