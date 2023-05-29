# 启动内核进程的汇编指令 通过trap实现
.section .text
KernelProcessEntry:
    .global KernelProcessEntry
    move a0,s1
    jalr s0
    move a0,s2
    jal KernelProcessExit

UserProcessEntry:
    # 这个入口是为了专门需要从特定函数启动的用户进程实现的
    .global UserProcessEntry
    move a0,s1
    jalr s0
    move a0,s2      # 进程结构体指针的参数
    move a1,s3      # 用户进程加载或启动地址的参数
    # 当用户进程的特定启动函数执行完成后
    # 将这个用户进程控制块作为参数传递
    # 并跳转到特定函数 转换到用户进程状态
    jal KerneltoUserProcess
