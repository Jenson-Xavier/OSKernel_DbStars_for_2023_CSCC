#ifndef __KOUT_HPP__
#define __KOUT_HPP__

#include <sbi.h>
#include <type.hpp>

void puts(const char * str);

enum COLOR
{
    Default=39,
    red=31,
    green=32,
    yellow=33,
    blue=34,
    purple=35,
    cyan=36,
};

class  KOUT
{
private:
    uint64 status; 

public:
    friend KOUT& endl(KOUT & kout);
    friend KOUT& clean(KOUT & kout);
    KOUT & operator<<(const char *);
    KOUT & operator<<(uint64);
    KOUT & operator<<(uint32);
    KOUT & operator<<(uint16);
    KOUT & operator<<(uint8);
    KOUT & operator<<(int64);
    KOUT & operator<<(int32);
    KOUT & operator<<(int16);
    KOUT & operator<<(int8);
    KOUT & operator<<(KOUT & (*p)(KOUT & kout));

    KOUT & operator[](COLOR _color);

};

KOUT& endl(KOUT & kout);
KOUT& clean(KOUT & kout);

extern KOUT kout;

#endif