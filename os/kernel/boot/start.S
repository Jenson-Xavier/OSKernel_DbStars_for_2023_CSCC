.section .text.entry

kernel_entry:
    .global kernel_entry

    la      t0,boot_sv39_page_table //配置satp寄存器中的值 
    srli    t0,t0,12                
    li      t1,8ull<<60             //sv39模式启动
    or      t0,t0,t1
    csrw    satp,t0
    sfence.vma//刷新快表机制 防止出错

    li t1,0xffffffff00000000//偏移项

    la sp,boot_stack_top
    add sp,sp,t1//映射系统栈

    la t0,main 
    add t0,t0,t1 //映射main函数

//    call putchar

    jalr t0
   

//    .global putchar
//putchar:
//    la a0,'a'
//    la a7,1
//    ecall


    .align 12

.section .bss.stack

    .global boot_stack
boot_stack:
    .space 4096


    .global boot_stack_top
boot_stack_top:

    .section .data
    .align 12
boot_sv39_page_table:
    .global boot_sv39_page_table
    .quad 0
    .quad 0
    .quad (0x80000ull<<10)|0xCF
    .quad 0
    .zero 8*504
    .quad (0x00000ull<<10)|0xCF
    .quad (0x40000ull<<10)|0xCF
    .quad (0x80000ull<<10)|0xCF
    .quad (0xc0000ull<<10)|0xCF