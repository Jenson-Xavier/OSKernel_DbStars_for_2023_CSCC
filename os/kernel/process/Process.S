# 启动内核进程的汇编指令 通过trap实现
.section .text
KernelProcessEntry:
    .global KernelProcessEntry
    move a0,s1
    jalr s0
    move a0,s2
    jal KernelProcessExit

UserProcessEntry:
    .global UserProcessEntry
    beqz s0,UserProcessEntry2
    # 下面这层逻辑事实上是并不符合用户进程的
    # 除了修改了特权级状态其他无异
    # 跳转了之后才算得上是用户进程和内核的隔离
    move a0,s1
    jalr s0
    move a0,s2
    jal KernelProcessExit

UserProcessEntry2:
    j TrapRet