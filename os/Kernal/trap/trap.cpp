#include <trap.hpp>
#include <clock.hpp>
#include <Riscv.h>
#include <kout.hpp>

extern "C"
{
    extern void trap_entry(void);

    TRAPFRAME *trap(TRAPFRAME *tf)
    {

        if ((long long)tf->cause < 0)
        {
            tf->cause <<= 1;
            tf->cause >>= 1;
            switch (tf->cause)
            {
            case IRQ_U_SOFT:
                break;
            case IRQ_S_SOFT:
                break;
            case IRQ_H_SOFT:
                break;
            case IRQ_M_SOFT:
                break;
            case IRQ_U_TIMER:
                break;
            case IRQ_S_TIMER:
                tickCount++;
                if (tickCount % 300000 == 0)
                {
                    tickCount = 0;
                    kout << '.';
                    set_next_tick();
                }

                break;
            case IRQ_H_TIMER:
                break;
            case IRQ_M_TIMER:
                break;
            case IRQ_U_EXT:
                break;
            case IRQ_S_EXT:
                break;
            case IRQ_H_EXT:
                break;
            case IRQ_M_EXT:
                break;
            }
        }
        else
        {
            switch (tf->cause)
            {
            case CAUSE_MISALIGNED_FETCH:
                break;
            case CAUSE_FETCH_ACCESS:
                break;
            case CAUSE_ILLEGAL_INSTRUCTION:
                break;
            case CAUSE_BREAKPOINT:
                break;
            case CAUSE_MISALIGNED_LOAD:
                break;
            case CAUSE_LOAD_ACCESS:
                break;
            case CAUSE_MISALIGNED_STORE:
                break;
            case CAUSE_STORE_ACCESS:
                break;
            case CAUSE_USER_ECALL:
                break;
            case CAUSE_SUPERVISOR_ECALL:
                break;
            case CAUSE_HYPERVISOR_ECALL:
                break;
            case CAUSE_MACHINE_ECALL:
                break;
            default:
                break;
            }
        }

        return tf;
    }
}

void trap_init()
{
    set_csr(sie, MIP_SSIP);
    set_csr(sie, MIP_SEIP);
    write_csr(sscratch, 0); // 说明中断是S态发生的
    write_csr(stvec, &trap_entry);
}

void show_reg(TRAPFRAME * tf)
{
    for (int i = 0; i < 32; i++)
    {
    kout[yellow]<<(*tf).reg.x[i]<<endl;
    }
    kout[yellow]<<(*tf).status<<endl;
    kout[yellow]<<(*tf).epc<<endl;
    kout[yellow]<<(*tf).badvaddr<<endl;
    kout[yellow]<<(*tf).cause<<endl;
    
}