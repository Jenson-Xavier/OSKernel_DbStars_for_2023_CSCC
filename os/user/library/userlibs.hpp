#ifndef __USERLIBS_HPP__
#define __USERLIBS_HPP__

#include <systemcall.hpp>

// 给用户程序测试使用的一些方便的输入输出函数

inline void u_puts(const char* str)
{
    while (*str)
    {
        u_putchar(*str);
        str++;
    }
    u_putchar('\n');
}

inline void u_puts(uint64 num)
{
    char buffer[21];
    int i = 0;
    if (num == 0)
    {
        u_putchar('0');
    }
    while (num)
    {
        buffer[i++] = num % 10;
        num /= 10;
    }
    i--;
    while (i >= 0)
    {
        u_putchar('0' + buffer[i--]);
    }
    u_putchar('\n');
}

inline void u_puts(uint32 num)
{
    u_puts((uint64)num);
}

inline void u_puts(uint16 num)
{
    u_puts((uint64)num);
}

inline void u_puts(uint8 num)
{
    u_puts((uint64)num);
}

inline void u_puts(int64 num)
{
    if (num < 0)
    {
        u_putchar('-');
        num = -num;
    }
    u_puts((uint64)num);
}

inline void u_puts(int32 num)
{
    u_puts((int64)num);
}

inline void u_puts(int16 num)
{
    u_puts((int64)num);
}

inline void u_puts(int8 num)
{
    u_puts((int64)num);
}

#endif