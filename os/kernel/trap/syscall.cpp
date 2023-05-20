#include <type.hpp>
#include <klib.hpp>
#include <memory.hpp>
#include <process.hpp>
#include <syscall.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <interrupt.hpp>
#include <synchronize.hpp>

// 实现系统调用函数
// 主要根据大赛要求依次进行实现

inline void SYS_putchar(char ch)
{
    putchar(ch);
}

inline char SYS_getchar()
{
    return getchar();
}

inline void SYS_exit(int ec)
{
    // 触发进程终止 无返回值
    // 输入参数ec为终止状态值
    proc_struct* cur_exited = pm.get_cur_proc();
    pm.exit_proc(cur_exited, ec);
    // 下面这条语句测试用 执行到这里说明有问题 上面已经保证进程被回收了
    kout[red] << "SYS_exit : reached unreachable branch!" << endl;
}



















// 系统调用方式遵循RISC-V ABI
// 即调用号存放在a7寄存器中 6个参数分别存放在a0-a5寄存器中 返回值存放在a0寄存器中
void trap_Syscall(TRAPFRAME* tf)
{
    // debug
    if (tf->reg.a7 >= 0)
    {
        // kout[green] << "Process's pid " << pm.get_cur_proc()->pid << " Syscall:" << endl;
        // kout[green] << "Syscall Number " << tf->reg.a7 << endl;
        // kout[green] << "Syscall name " << get_syscall_name(tf->reg.a7) << endl;
    }
    switch (tf->reg.a7)
    {
    case Sys_putchar:
        SYS_putchar(tf->reg.a0);
        break;
    case Sys_getchar:
        tf->reg.a0 = SYS_getchar();
        break;
    case Sys_getcwd:
        break;
    case Sys_pipe2:
        break;
    case Sys_dup:
        break;
    case Sys_dup3:
        break;
    case Sys_chdir:
        break;
    case Sys_openat:
        break;
    case Sys_close:
        break;
    case Sys_getdents64:
        break;
    case Sys_read:
        break;
    case Sys_write:
        break;
    case Sys_linkat:
        break;
    case Sys_unlinkat:
        break;
    case Sys_mkdirat:
        break;
    case Sys_umount2:
        break;
    case Sys_mount:
        break;
    case Sys_fstat:
        break;
    case Sys_clone:
        break;
    case Sys_execve:
        break;
    case Sys_wait4:
        break;
    case Sys_exit:
        SYS_exit(tf->reg.a0);
        break;
    case Sys_getppid:
        break;
    case Sys_getpid:
        break;
    case Sys_brk:
        break;
    case Sys_munmap:
        break;
    case Sys_mmap:
        break;
    case Sys_times:
        break;
    case Sys_uname:
        break;
    case Sys_sched_yield:
        break;
    case Sys_gettimeofday:
        break;
    case Sys_nanosleep:
        break;
    default:
        // 遇到了未定义的系统调用号
        // 这里的处理就是输出相关信息即可
        kout[yellow] << "The Syscall has not defined!" << endl;
        kout[yellow] << "Current Process's pid is " << pm.get_cur_proc()->pid << endl;
        if (pm.get_cur_proc()->ku_flag == 2)
        {
            // 是用户进程发起的系统调用
            // 那么这个用户进程遇到这种情况应该直接终止
            pm.exit_proc(pm.get_cur_proc(), -1);
            kout[red] << "trap_Syscall : reached unreachable branch!" << endl;
        }
        break;
    }
}