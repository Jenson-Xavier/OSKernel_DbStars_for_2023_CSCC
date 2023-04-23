#ifndef _SBI_HPP_
#define _SBI_HPP_

#include <type.hpp>

#define sbi_call(ext, fid, arg0, arg1, arg2, arg3, arg4, arg5)              \
    ({                                                                      \
        register unsigned long a0 asm("a0") = (uint32)(arg0);               \
        register unsigned long a1 asm("a1") = (uint32)(arg1);               \
        register unsigned long a2 asm("a2") = (uint32)(arg2);               \
        register unsigned long a3 asm("a3") = (uint32)(arg3);               \
        register unsigned long a4 asm("a4") = (uint32)(arg4);               \
        register unsigned long a5 asm("a5") = (uint32)(arg5);               \
        register unsigned long a6 asm("a6") = (uint32)(fid);                \
        register unsigned long a7 asm("a7") = (uint32)(ext);                \
        asm volatile("ecall"                                                \
                     : "+r"(a0), "+r"(a1)                                   \
                     : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7) \
                     : "memory");                                           \
        a0;                                                                 \
    })

#define sbi_putchar(arg0) sbi_call(1, 0, arg0, 0, 0, 0, 0, 0)
#define sbi_getchar() sbi_call(2, 0, 0, 0, 0, 0, 0, 0)
#define sbi_set_time(t) sbi_call(0, 0, t, 0, 0, 0, 0, 0)
#define sbi_shutdown() sbi_call(8, 0, 0, 0, 0, 0, 0, 0)

#define sbi_get_time()            \
    ({                            \
        uint64 re;                \
        asm volatile("rdtime %0"  \
                     : "=r"(re)); \
        re;                       \
    })

#endif