.section .text.entry

    .global kernel_entry
kernel_entry:
    la sp,boot_stack_top
    tail main
   
    .align 12

.section .bss.stack

    .global boot_stack
boot_stack:
    .space 4096


    .global boot_stack_top
boot_stack_top:
