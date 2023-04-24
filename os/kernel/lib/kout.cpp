#include <kout.hpp>

KOUT kout;

void puts(const char* str)
{
    while (*str)
    {
        sbi_putchar(*str);
        str++;
    }

}

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

KOUT& KOUT::hex(uint64 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 8;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return *this;
}

KOUT& KOUT::hex(uint32 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 4;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return *this;
}

KOUT& KOUT::hex(uint16 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 2;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return *this;
}

KOUT& KOUT::hex(uint8 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 1;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return *this;
}

KOUT& KOUT::hex(int64 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 8;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return *this;
}

KOUT& KOUT::hex(int32 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 4;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return *this;
}

KOUT& KOUT::hex(int16 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 2;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
    return *this;
}

KOUT& KOUT::hex(int8 data, bool is_large)
{
    *this << "0x";
    int operator_cnt = 1;
    while (operator_cnt--) {
        int shiftdigits = operator_cnt * 8;
        outHex((unsigned char)(data >> shiftdigits), is_large);
    }
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
