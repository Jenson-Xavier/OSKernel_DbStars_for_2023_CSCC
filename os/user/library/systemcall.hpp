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

#endif