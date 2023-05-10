#ifndef __SYSCALL_HPP__
#define __SYSCALL_HPP__

// 实现系统调用部分

// 通过枚举类型实现对系统调用号的绑定
enum syscall_ID
{
    Sys_putchar = -1,
    Sys_getchar = -2,

    // ...
};

// 将对应的系统调用号返回其系统调用的名字
inline const char* get_syscall_name(int syscall_id)
{
    switch (syscall_id)
    {
    case Sys_putchar:
        return "Sys_putchar";
    case Sys_getchar:
        return "Sys_getchar";
    // ...
    
    default:
        return "";
    }
}

// trap中实现系统调用的主体函数
int trap_syscall(TRAPFRAME* tf);

#endif