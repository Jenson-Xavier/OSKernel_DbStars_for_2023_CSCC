#include <trap.hpp>
#include <clock.hpp>
#include <Riscv.h>
#include <kout.hpp>
#include <process.hpp>
#include <vmm.hpp>

extern "C"
{
    extern void trap_entry(void);

    // 注意这里的trap函数返回值设置为TRAPFRAME指针
    // 从而可以方便利用trap进行进程的调度
    TRAPFRAME* trap(TRAPFRAME* tf)
    {
        // 中断
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
                // S态的时钟中断
                tickCount++;
                if (tickCount % 100 == 0)
                {
                    tickCount = 0;
                    kout << '.';
                }
                set_next_tick();

                //用时钟中断来触发进程的调度
                if (tickCount % 100 == 0)
                {
                    // kout[yellow] << "cur_proc's pid is : " << pm.get_cur_proc()->pid << endl;
                    // kout[yellow] << "cur proc cnt is : " << pm.get_proc_count() << endl;
                    // 每100个时钟周期进行一次进程RR调度
                    return pm.proc_scheduler(tf);
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
            // 异常
            switch (tf->cause)
            {
            case CAUSE_MISALIGNED_FETCH:
                break;
            case CAUSE_FETCH_ACCESS:
                break;
            case CAUSE_ILLEGAL_INSTRUCTION:
                break;
            case CAUSE_BREAKPOINT:
                // 执行ebreak调用触发断点异常 从而实现进程退出的回收
                // 通过内联汇编已经将参数中的a7寄存器设置为1
                switch (tf->reg.a7)
                {
                case 1:
                    kout[blue] << "Current Proc's PID is " << pm.get_cur_proc()->pid << " and Exit!" << endl;
                    // pm.print_all_list();
                    return pm.proc_scheduler(tf);
                }
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
            case CAUSE_STORE_PAGE_FAULT:
                kout[yellow]<<"PAFE_FAULT"<<endl;
                TrapFunc_PageFault(tf);
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

void show_reg(TRAPFRAME* tf)
{
    for (int i = 0; i < 32; i++)
    {
        kout[yellow] << (*tf).reg.x[i] << endl;
    }
    kout[yellow] << (*tf).status << endl;
    kout[yellow] << (*tf).epc << endl;
    kout[yellow] << (*tf).badvaddr << endl;
    kout[yellow] << (*tf).cause << endl;
}