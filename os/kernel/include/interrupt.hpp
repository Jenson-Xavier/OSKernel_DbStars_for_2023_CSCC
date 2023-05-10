#ifndef __INTERRUPT_HPP__
#define __INTERRUPT_HPP__

#include <Riscv.h>

// 中断使能的基本函数
void interrupt_enable();
void interrupt_disable();

// 判断现在的中断是否打开
inline bool is_intr_enable()
{
    if (read_csr(sstatus) & SSTATUS_SIE)
    {
        return true;
    }
    return false;
}

// 为了设计原语的需要 
// 设计中断使能需要的保存状态等函数和封装
static inline bool _interrupt_save()
{
    if (read_csr(sstatus) & SSTATUS_SIE)
    {
        interrupt_disable();
        return 1;           // 相当于上一次中断使能位的状态为1
    }
    return 0;               // 相当于上一次中断使能位的状态为0
}

static inline void _interrupt_restore(bool intr_flag)
{
    if (intr_flag)
    {
        interrupt_enable(); // 若上一次本身就是0就无需什么操作
    }
}

// 宏定义方便使用
// 参考借鉴了ucore实现
#define intr_save(intr_flag)        do { intr_flag = _interrupt_save(); } while (0)
#define intr_restore(intr_flag)     _interrupt_restore(intr_flag);

#endif