#ifndef __SYSTEMCALL_HPP__
#define __SYSTEMCALL_HPP__

// 给用户提供的系统调用接口
#include "../../kernel/include/syscall.hpp"

inline void u_putchar(char ch)
{
    syscall(Sys_putchar, ch);
}

inline char u_getchar()
{
    char ret = syscall(Sys_getchar);
    return ret;
}

inline void u_exit(int ec = 0)
{
    syscall(Sys_exit, ec);
}

inline int u_clone(int flags, void* stack, int ptid, int tls, int ctid)
{
    int pret = syscall(Sys_clone, flags, (uint64)stack, ptid, tls, ctid);
    return pret;
}

// 测试clone & wait

extern "C"
{
    extern int __clone(int (*func)(void), void* stack, int flags, void* arg, ...);
}

inline int clone(int (*fn)(void), void* arg, void* stack, int stack_size, unsigned long flags)
{
    if (stack != nullptr)
    {
        stack = (void*)((uint64)stack + stack_size);
    }
    int ret = __clone(fn, stack, flags, 0, 0, 0);
    return ret;
    //return syscall(SYS_clone, fn, stack, flags, NULL, NULL, NULL);
}

inline int waitpid(int pid, int* wstatus, int options)
{
    int ret = syscall(Sys_wait4, pid, (uint64)wstatus, options);
    return ret;
}

inline int wait(int* wstatus)
{
    int ret = waitpid((int)-1, wstatus, 0);
    return ret;
}

inline int getppid()
{
    int ret = syscall(Sys_getppid);
    return ret;
}

inline int getpid()
{
    int ret = syscall(Sys_getpid);
    return ret;
}

inline int times(void* mytimes)
{
    return syscall(Sys_times, (uint64)mytimes);
}

inline int uname(void* buf)
{
    return syscall(Sys_uname, (uint64)buf);
}

typedef struct
{
    uint64 sec;  // 自 Unix 纪元起的秒数
    uint64 usec; // 微秒数
} TimeVal;

struct timespec
{
    long tv_sec;                    // 秒
    long tv_nsec;                   // 纳秒 范围0~999999999
};

inline int sleep(unsigned long long time)
{
    timespec tv = { .tv_sec = time, .tv_nsec = 0 };
    if (syscall(Sys_nanosleep, (uint64)&tv, (uint64)&tv)) return tv.tv_sec;
    return 0;
}

inline int sys_get_time(TimeVal* ts, int tz)
{
    return syscall(Sys_gettimeofday, (uint64)ts, tz);
}

inline int64 get_time()
{
    TimeVal time;
    int err = sys_get_time(&time, 0);
    if (err == 0)
    {
        return ((time.sec & 0xffff) * 1000 + time.usec / 1000);
    }
    else
    {
        return -1;
    }
}

inline int execve(const char *name, char *const argv[], char *const argp[])
{
    return syscall(Sys_execve, (uint64)name, (uint64)argv, (uint64)argp);
}

inline int sched_yield(void)
{
    return syscall(Sys_sched_yield);
}

inline int fork(void)
{
    return syscall(Sys_clone, 17, 0);
}

#endif