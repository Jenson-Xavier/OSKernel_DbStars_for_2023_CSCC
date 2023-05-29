#ifndef __KSTRING_HPP__
#define __KSTRING_HPP__

// 一开始并未考虑实现
// 在实现进程过程中发现有必要 暂时简单实现几个常用的经典cstring函数 为自己的kstring

#include <type.hpp>
#include <sbi.h>

// 做一个简单的封装
inline void putchar(char ch)
{
    sbi_putchar(ch);
}

inline void puts(const char* str)
{
    while (*str)
    {
        sbi_putchar(*str);
        str++;
    }
}

inline int getchar()
{
    int ret_ch;
    while (1)
    {
        ret_ch = sbi_getchar();
        if (ret_ch != -1)
        {
            break;
        }
    }
    return ret_ch;
}

inline bool isdigit(char c)
{
    if (c <= '9' && c >= '0')
        return true;
    return false;
}

inline int gets(char buf[], int buffersize)
{
    int i = 0;
    while (i < buffersize)
    {
        char ch = getchar();
        if (ch == '\n' || ch == '\r' || ch == '\0' || ch == -1)
        {
            putchar('\n');
            buf[i++] = 0;
            break;
        }
        else
        {
            putchar(ch);
            buf[i++] = ch;
        }
    }
    return i;
}

inline bool isInStr(char tar, const char* src)
{
    while (*src)
    {
        if (tar == *src)
            return true;

        src++;
    }
    return false;
}

uint64 strlen(const char* s);

void strcpy(char* dst, const char* src);

char* strcpy_s(char* dst, const char* src);

char* strcpy_no_end(char* dst, const char* src);

int strcmp(const char* s1, const char* s2);

void* memset(void* s, char ch, uint64 size);

void* memcpy(void* dst, const char* src, uint64 size);

#endif