#ifndef __SYSCALL_HPP__
#define __SYSCALL_HPP__

#include <trap.hpp>
#include <klib.hpp>

// 实现系统调用部分
// 主要按照大赛对系统调用的要求进行实现

// 通过枚举类型实现对系统调用号的绑定
enum syscall_ID
{
    Sys_putchar = 1000,             // 自定义系统调用号
    Sys_getchar = 1001,

    Sys_getcwd = 17,                // 根据大赛要求需要编写的
    Sys_pipe2 = 59,
    Sys_dup = 23,
    Sys_dup3 = 24,
    Sys_chdir = 49,
    Sys_openat = 56,
    Sys_close = 57,
    Sys_getdents64 = 61,
    Sys_read = 63,
    Sys_write = 64,
    Sys_linkat = 37,
    Sys_unlinkat = 35,
    Sys_mkdirat = 34,
    Sys_umount2 = 39,
    Sys_mount = 40,
    Sys_fstat = 80,
    Sys_clone = 220,
    Sys_execve = 221,
    Sys_wait4 = 260,
    Sys_exit = 93,
    Sys_getppid = 173,
    Sys_getpid = 172,
    Sys_brk = 214,
    Sys_munmap = 215,
    Sys_mmap = 222,
    Sys_times = 153,
    Sys_uname = 160,
    Sys_sched_yield = 124,
    Sys_gettimeofday = 169,
    Sys_nanosleep = 101
};

// 将对应的系统调用号返回其系统调用的名字
inline const char* get_syscall_name(int syscall_id)
{
    switch (syscall_id)
    {
    case Sys_putchar:
        return "SYS_putchar";
    case Sys_getchar:
        return "SYS_getchar";
    case Sys_getcwd:
        return "SYS_getcwd";
    case Sys_pipe2:
        return "SYS_pipe2";
    case Sys_dup:
        return "SYS_dup";
    case Sys_dup3:
        return "SYS_dup3";
    case Sys_chdir:
        return "SYS_chdir";
    case Sys_openat:
        return "SYS_openat";
    case Sys_close:
        return "SYS_close";
    case Sys_getdents64:
        return "SYS_getdents64";
    case Sys_read:
        return "SYS_read";
    case Sys_write:
        return "SYS_write";
    case Sys_linkat:
        return "SYS_linkat";
    case Sys_unlinkat:
        return "SYS_unlinkat";
    case Sys_mkdirat:
        return "SYS_mkdirat";
    case Sys_umount2:
        return "SYS_umount2";
    case Sys_mount:
        return "SYS_mount";
    case Sys_fstat:
        return "SYS_fstat";
    case Sys_clone:
        return "SYS_clone";
    case Sys_execve:
        return "SYS_execve";
    case Sys_wait4:
        return "SYS_wait4";
    case Sys_exit:
        return "SYS_exit";
    case Sys_getppid:
        return "SYS_getppid";
    case Sys_getpid:
        return "SYS_getpid";
    case Sys_brk:
        return "SYS_brk";
    case Sys_munmap:
        return "SYS_munmap";
    case Sys_mmap:
        return "SYS_mmap";
    case Sys_times:
        return "SYS_times";
    case Sys_uname:
        return "SYS_uname";
    case Sys_sched_yield:
        return "SYS_sched_yield";
    case Sys_gettimeofday:
        return "SYS_gettimeofday";
    case Sys_nanosleep:
        return "SYS_nanosleep";
    default:
        return "";
    }
}

// trap中实现系统调用的主体函数
// 系统调用在trap中的转发
void trap_Syscall(TRAPFRAME* tf);

// 进行系统调用的主接口函数 内联汇编实现
// 遵循RISC-V ABI系统调用规则
// 这个接口主要是给用户态调用的 注意ecall命令的转发
inline uint64 syscall(uint64 call_id,
    uint64 a0 = 0, uint64 a1 = 0, uint64 a2 = 0, uint64 a3 = 0, uint64 a4 = 0, uint64 a5 = 0)
{
    uint64 ret;
    asm volatile
        (
            "ld a0,%1\n"        // 依次存放6个参数
            "ld a1,%2\n"
            "ld a2,%3\n"
            "ld a3,%4\n"
            "ld a4,%5\n"
            "ld a5,%6\n"
            "ld a7,%7\n"        // 存放系统调用号
            "ecall\n"           // U态ecall触发异常进入系统调用的trap转发
            "sd a0,%0"          // sd双字存储 将调用之后的返回值提取到ret中
            :"=m"(ret)
            : "m"(a0), "m"(a1), "m"(a2), "m"(a3), "m"(a4), "m"(a5), "m"(call_id)
            : "memory"
            );
    return ret;
}

#endif