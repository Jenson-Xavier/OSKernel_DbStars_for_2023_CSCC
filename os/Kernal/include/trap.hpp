#ifndef __TRAP_HPP__
#define __TRAP_HPP__

#include <type.hpp>

struct CSR
{
    union
    {
        uint64 x[32];
        struct
        {
            uint64 zero, ra, sp, gp, tp,
                    t0, t1, t2,
                    s0, s1,
                    a0, a1, a2, a3, a4, a5, a6, a7,
                    s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
                    t3, t4, t5, t6;
        };
    };
    uint64 & operator[](int i)
    {
        return x[i];
    }
};

struct TRAPFRAME
{
    CSR reg;
    uint64 status,
            epc,
            badvaddr,
            cause;
};


extern"C" TRAPFRAME * trap(TRAPFRAME *tf);
void trap_init();
void show_reg(TRAPFRAME * tf);

#endif