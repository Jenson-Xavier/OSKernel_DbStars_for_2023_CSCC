#ifndef __KOUT_HPP__
#define __KOUT_HPP__

#include <sbi.h>
#include <type.hpp>

// 在kout.hpp中定义的宏名结合相关定义
// 可以使用kout << Hex(x)的方式按16进制输出x
#define Hex KOUT::hex
#define Memory KOUT::memory

void puts(const char* str);

enum COLOR
{
    Default = 39,
    red = 31,
    green = 32,
    yellow = 33,
    blue = 34,
    purple = 35,
    cyan = 36,
};

class KOUT
{
private:
    uint64 status;

    // 通过对unsigned char类型的变量 8位 打印它的16进制形式
    inline static void outHex(unsigned char x, bool is_large = true)
    {
        if ((x >> 4) <= 9)
        {
            // 输出高4位
            sbi_putchar((x >> 4) + '0');
        }
        else
        {
            sbi_putchar((x >> 4) - 10 + (is_large ? 'A' : 'a'));
        }
        if ((x & 0x0f) <= 9)
        {
            // 输出低4位
            sbi_putchar((x & 0x0f) + '0');
        }
        else
        {
            sbi_putchar((x & 0x0f) - 10 + (is_large ? 'A' : 'a'));
        }
    }

public:
    friend KOUT& endl(KOUT& kout);
    friend KOUT& clean(KOUT& kout);
    KOUT& operator<<(const char*);
    KOUT& operator<<(uint64);
    KOUT& operator<<(uint32);
    KOUT& operator<<(uint16);
    KOUT& operator<<(uint8);
    KOUT& operator<<(int64);
    KOUT& operator<<(int32);
    KOUT& operator<<(int16);
    KOUT& operator<<(int8);
    KOUT& operator<<(KOUT& (*p)(KOUT& kout));
    KOUT& operator<<(void* p);

    KOUT& operator[](COLOR _color);

    // 简单实现对各种数据类型的十六进制的输出 方便进行调试
    // 调试功能的需求 不是内核核心功能的关键
    // 就采用比较简单的实现了 重复的代码工作
    static void* hex(uint64, bool is_large = true);
    static void* hex(uint32, bool is_large = true);
    static void* hex(uint16, bool is_large = true);
    static void* hex(uint8, bool is_large = true);

    // 简单实现给定一段内存区域的起始地址和结束地址打印内存片的内容
    // 最方便的实现就是按字节打印
    // 继续使用类似上面hex的方式实现
    static void* memory(void*, void*, uint32 show = 1);
    static void* memory(void*, uint64 size);
};

KOUT& endl(KOUT& kout);
KOUT& clean(KOUT& kout);

extern KOUT kout;

#endif