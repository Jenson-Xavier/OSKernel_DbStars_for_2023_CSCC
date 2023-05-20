#include <trap.hpp>
#include <clock.hpp>
#include <Riscv.h>
#include <kout.hpp>
#include <process.hpp>
#include <memory.hpp>
#include <syscall.hpp>

// 中断名字数组 便于打印调试
static const char* trap_intr_code_name[16] =
{
    "InterruptCode_0"							,
    "InterruptCode_SupervisorSoftwareInterrupt" ,
    "InterruptCode_2"							,
    "InterruptCode_MachineSoftwareInterrupt"	,
    "InterruptCode_4"							,
    "InterruptCode_SupervisorTimerInterrupt"	,
    "InterruptCode_6"							,
    "InterruptCode_MachineTimerInterrupt"		,
    "InterruptCode_8"							,
    "InterruptCode_SupervisorExternalInterrupt"	,
    "InterruptCode_10"							,
    "InterruptCode_MachineExternalInterrupt"	,
    "InterruptCode_12"							,
    "InterruptCode_13"							,
    "InterruptCode_14"							,
    "InterruptCode_15"
};

// 异常名字数组 便于打印调试
static const char* trap_excp_code_name[16] =
{
    "ExceptionCode_InstructionAddressMisaligned" ,
    "ExceptionCode_InstructionAccessFault"       ,
    "ExceptionCode_IllegalInstruction"           ,
    "ExceptionCode_BreakPoint"                   ,
    "ExceptionCode_LoadAddressMisaligned"        ,
    "ExceptionCode_LoadAccessFault"              ,
    "ExceptionCode_StoreAddressMisaligned"       ,
    "ExceptionCode_StoreAccessFault"             ,
    "ExceptionCode_UserEcall"	                 ,
    "ExceptionCode_SupervisorEcall"              ,
    "ExceptionCode_HypervisorEcall"              ,
    "ExceptionCode_MachineEcall"                 ,
    "ExceptionCode_InstructionPageFault"         ,
    "ExceptionCode_LoadPageFault"                ,
    "ExceptionCode_14"                           ,
    "ExceptionCode_StorePageFault"
};

void trap_fail_info(TRAPFRAME* tf)
{
    // 需要对还没有实现的中断异常设置这个函数
    // 能极大地方便我们的调试工作
    kout[red] << "Trap Failed !" << endl;
    kout[green] << "Type:" << (void*)tf->cause << endl;
    kout[green] << "Name:";
    if ((int64)tf->cause < 0)
    {
        kout[green] << trap_intr_code_name[tf->cause << 1 >> 1] << endl;
    }
    else
    {
        kout[green] << trap_excp_code_name[tf->cause] << endl;
    }
    kout[green] << "tval:" << (void*)tf->badvaddr << endl;
    kout[green] << "status:" << (void*)tf->status << endl;
    kout[green] << "epc:" << (void*)tf->epc << endl;
}

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
                    // kout << '.';
                }
                set_next_tick();

                //用时钟中断来触发进程的调度
                if (tickCount % 100 == 0 || imme_schedule)
                {
                    // kout[green] << imme_schedule << endl;
                    // kout[yellow] << "cur_proc's pid is : " << pm.get_cur_proc()->pid << endl;
                    // kout[yellow] << "cur_proc's name is " << pm.get_cur_proc()->name << endl;
                    // kout[yellow] << "cur proc cnt is : " << pm.get_proc_count() << endl;
                    // 每100个时钟周期进行一次进程RR调度
                    // 或者imme_schedule为真时立即调度切换进程实现同步原语下的进程调度
                    if (tickCount % 100 != 0 && imme_schedule == 1)
                    {
                        // 为了防止因为imme_schedule进入调度导致每次时钟中断都会触发调度
                        // 如果是因为imme_schedule进入的调度 立马置0
                        // imme_schedule的逻辑暂时也就是单次触发的逻辑
                        imme_schedule = 0;
                    }
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
            case CAUSE_FETCH_ACCESS:
            case CAUSE_ILLEGAL_INSTRUCTION:
                trap_fail_info(tf);
                break;
            case CAUSE_BREAKPOINT:
                // 执行ebreak调用触发断点异常 从而实现进程退出的回收
                // 通过内联汇编已经将参数中的a7寄存器设置为1
                switch (tf->reg.a7)
                {
                case 1:
                    kout[blue] << "Current Process's PID is " << pm.get_cur_proc()->pid << " and Exit!" << endl;
                    // pm.print_all_list();
                    return pm.proc_scheduler(tf);
                }
                break;
            case CAUSE_MISALIGNED_LOAD:
            case CAUSE_LOAD_ACCESS:
            case CAUSE_MISALIGNED_STORE:
            case CAUSE_STORE_ACCESS:
                trap_fail_info(tf);
                break;
            case CAUSE_USER_ECALL:
                // 系统调用部分 用户触发USER_ECALL异常
                trap_Syscall(tf);
                tf->epc += 4;           // 将sepc指向ecall的下一条指令地址处
                break;
            case CAUSE_SUPERVISOR_ECALL:
            case CAUSE_HYPERVISOR_ECALL:
            case CAUSE_MACHINE_ECALL:
                trap_fail_info(tf);
                break;
            case 12:
            case 13:
            case CAUSE_STORE_PAGE_FAULT:
                // 缺页中断异常
                // kout[yellow] << "PAGE_FAULT" << endl;
                trap_PageFault(tf);
                break;
            default:
                trap_fail_info(tf);
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