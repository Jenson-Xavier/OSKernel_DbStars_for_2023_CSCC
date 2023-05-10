#include <kout.hpp>
#include <kstring.hpp>

KOUT kout;

KOUT& KOUT::operator<<(const char* str)
{
    puts(str);
    return *this;
}

KOUT& KOUT::operator<<(uint64 num)
{
    char buffer[21];
    int i = 0;
    if (num == 0)
    {
        sbi_putchar('0');
        return *this;
    }

    while (num)
    {
        buffer[i++] = num % 10;
        num /= 10;
    }
    i--;
    while (i >= 0)
    {
        sbi_putchar('0' + buffer[i--]);
    }

    return *this;
}

KOUT& KOUT::operator<<(uint32 num)
{
    return *this << (uint64)num;
}

KOUT& KOUT::operator<<(uint16 num)
{
    return *this << (uint64)num;
}

KOUT& KOUT::operator<<(uint8 num)
{
    return *this << (uint64)num;
}

KOUT& KOUT::operator<<(int64 num)
{
    if (num < 0)
    {
        sbi_putchar('-');
        num = -num;
    }
    return *this << (uint64)num;
}

KOUT& KOUT::operator<<(int32 num)
{
    return *this << (int64)num;
}

KOUT& KOUT::operator<<(int16 num)
{
    return *this << (int64)num;
}

KOUT& KOUT::operator<<(int8 num)
{
    sbi_putchar(num);
    return *this;
}

KOUT& KOUT::operator[](COLOR _color)
{
    *this << "\e[" << (uint8)_color << 'm';

    return *this;
}

void* KOUT::hex(uint64 data, bool is_large)
{
    puts("0x");
    int operator_cnt = 8;
    while (operator_cnt--)
    {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return nullptr;
}

void* KOUT::hex(uint32 data, bool is_large)
{
    puts("0x");
    int operator_cnt = 4;
    while (operator_cnt--)
    {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return nullptr;
}

void* KOUT::hex(uint16 data, bool is_large)
{
    puts("0x");
    int operator_cnt = 2;
    while (operator_cnt--)
    {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return nullptr;
}

void* KOUT::hex(uint8 data, bool is_large)
{
    puts("0x");
    int operator_cnt = 1;
    while (operator_cnt--)
    {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return nullptr;
}

void* KOUT::memory(void* start_address, void* end_address, uint32 show)
{
    // 按照字节打印实现
    unsigned char* startptr = (unsigned char*)start_address;
    unsigned char* endptr = (unsigned char*)end_address;
    // show控制每次连续多少个字节进行显示
    // 默认显示一个字节一个字节的内容
    // 一般取决与想要显示的数据类型大小
    // 注意RISCV架构下是采用的大端表示方式
    int show_cnt = 0;
    while (startptr < endptr)
    {
        if (show_cnt == 0)
        {
            kout << KOUT::hex((uint64)startptr) << ": 0x";
        }
        outHex(*startptr);
        if (++show_cnt == show)
        {
            kout << endl;
            show_cnt = 0;
        }
        startptr++;
    }
    return nullptr;
}

KOUT& KOUT::operator<<(void* p)
{
    return *this;
}

KOUT& endl(KOUT& kout)
{
    kout << '\n';
    kout << "\e[0m";
    return kout;
}

KOUT& clean(KOUT& kout)
{
    kout << "\e[0m";
    return kout;
}

KOUT& KOUT::operator<<(KOUT& (*p)(KOUT& kout))
{
    return p(*this);
}
