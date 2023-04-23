#include <kout.hpp>

KOUT kout;

void puts(const char * str)
{
    while (*str)
    {
        sbi_putchar(*str);
        str++;
    }
    
}

KOUT & KOUT::operator<<(const char * str)
{
    puts(str);
    return * this;
}

KOUT & KOUT::operator<<(uint64 num)
{
    char buffer[21];
    int i=0;
    if (num==0)
    {
        sbi_putchar('0');
        return *this;
    }
    
    while (num)
    {
        buffer[i++]=num%10;
        num/=10;
    }
    i--;
    while (i>=0)
    {
        sbi_putchar('0'+buffer[i--]);
    }
    
    return *this; 
    
}
KOUT & KOUT::operator<<(uint32 num)
{
    return *this<<(uint64)num;
}
KOUT & KOUT::operator<<(uint16 num)
{
    return *this<<(uint64)num;
}
KOUT & KOUT::operator<<(uint8 num)
{
    return *this<<(uint64)num;
}
KOUT & KOUT::operator<<(int64 num)
{
    if (num<0)
    {
        sbi_putchar('-');
        num=-num;
    }
    return *this << (uint64)num;
}
KOUT & KOUT::operator<<(int32 num)
{
    return *this << (int64)num;
}
KOUT & KOUT::operator<<(int16 num)
{
    return *this << (int64)num;
}
KOUT & KOUT::operator<<(int8 num)
{
    sbi_putchar(num);
    return *this;
}


KOUT & KOUT::operator[](COLOR _color)
{
    *this<<"\e["<<(uint8)_color<<'m';

    return *this;
}

KOUT& endl(KOUT & kout)
{
    kout<<'\n';
    kout<<"\e[0m" ;
    return kout;
}
KOUT& clean(KOUT & kout)
{
    kout<<"\e[0m";
    return kout;
}


KOUT & KOUT::operator<<(KOUT & (*p)(KOUT & kout))
{
    return p(*this);
}
