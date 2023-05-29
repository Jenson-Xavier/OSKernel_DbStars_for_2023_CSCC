#include <type.hpp>
#include <klib.hpp>
#include <memory.hpp>
#include <process.hpp>
#include <syscall.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <interrupt.hpp>
#include <synchronize.hpp>

// 实现系统调用函数
// 主要根据大赛要求依次进行实现

inline void SYS_putchar(char ch)
{
    putchar(ch);
}

inline char SYS_getchar()
{
    return getchar();
}

inline void SYS_exit(int ec)
{
    // 触发进程终止 无返回值
    // 输入参数ec为终止状态值

    proc_struct* cur_exited = pm.get_cur_proc();
    if (cur_exited->fa != nullptr)
    {
        // kout << "The proc has Father proc whose pid is " << cur_exited->fa->pid << endl;    
    }
    pm.exit_proc(cur_exited, ec);
    // 下面这条语句测试用 执行到这里说明有问题 上面已经保证进程被回收了
    kout[red] << "SYS_exit : reached unreachable branch!" << endl;
}
















inline int SYS_clone(TRAPFRAME* tf, int flags, void* stack, int ptid, int tls, int ctid)
{
    // 创建一个子进程系统调用
    // 成功返回子进程的线程ID 失败返回-1
    // 通过stack来进行fork还是clone
    // fork则是赋值父进程的代码空间 clone则是创建一个新的进程
    // tf参数是依赖于实现的 用来启动进程的

    constexpr uint64 SIGCHLD = 17;      // flags参数
    int pid_ret = -1;
    bool intr_flag;
    intr_save(intr_flag);               // 开关中断 操作的原语性
    proc_struct* cur_proc = pm.get_cur_proc();
    proc_struct* create_proc = pm.alloc_proc();
    if (create_proc == nullptr)
    {
        kout[red] << "SYS_clone alloc proc Fail!" << endl;
        return pid_ret;
    }
    pm.init_proc(create_proc, 2, flags);
    pid_ret = create_proc->pid;
    pm.set_proc_kstk(create_proc, nullptr, cur_proc->kstacksize);
    if (stack == nullptr)
    {
        // 根据clone的系统调用
        // 当stack为nullptr时 此时的系统调用和fork一致
        // 子进程和父进程执行的一段相同的二进制代码
        // 其实就是上下文的拷贝
        // 子进程的创建
        VMS* vms = (VMS*)kmalloc(sizeof(VMS));
        vms->init();
        // vms->create_from_vms(cur_proc->vms);    // 此函数还没实现!!!
        memcpy((void*)((TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1),
            (const char*)tf, sizeof(TRAPFRAME));
        create_proc->context = (TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1;
        create_proc->context->epc += 4;
        create_proc->context->reg.a0 = 0;
    }
    else
    {
        // 此时就是相当于clone
        // 创建一个新的线程 与内存间的共享有关
        // 线程thread的创建
        // 这里其实隐含很多问题 涉及到指令执行流这些 需要仔细体会
        // 在大赛要求和本内核实现下仅仅是充当创建一个函数线程的作用
        pm.set_proc_vms(create_proc, cur_proc->vms);
        memcpy((void*)((TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1),
            (const char*)tf, sizeof(TRAPFRAME));
        create_proc->context = (TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1;
        create_proc->context->epc += 4;
        create_proc->context->reg.sp = (uint64)stack;
        create_proc->context->reg.a0 = 0;           // clone里面子线程去执行函数 父线程返回
    }
    pm.copy_other_proc(create_proc, cur_proc);
    if (flags & SIGCHLD)
    {
        // clone的是子进程
        pm.set_fa_proc(create_proc, cur_proc);
    }
    pm.switchstat_proc(create_proc, Proc_ready);
    intr_restore(intr_flag);
    return pid_ret;
}

inline int SYS_execve(const char* path, char* const argv[], char* const envp[])
{
    // 执行一个指定的程序的系统调用
    // 关键是利用解析ELF文件创建进程来实现
    // 这里需要文件系统提供的支持
    // argv参数是程序参数 envp参数是环境变量的数组指针 暂时未使用
    // 执行成功跳转执行对应的程序 失败则返回-1

    return -1;
}

inline int SYS_wait4(int pid, int* status, int options)
{
    // 等待进程改变状态的系统调用
    // 这里的改变状态主要是指等待子进程结束
    // pid是指定进程的pid -1就是等待所有子进程
    // status是接受进程退出状态的指针
    // options是指定选项 可以参考linux系统调用文档
    // 这里主要有WNOHANG WUNTRACED WCONTINUED
    // WNOHANG:return immediately if no child has exited.
    // 成功返回进程ID

    constexpr int WNOHANG = 1;                      // 目前暂时只需要这个option
    proc_struct* cur_proc = pm.get_cur_proc();
    if (cur_proc->fir_child == nullptr)
    {
        // 当前父进程没有子进程
        // 一般调用此函数不存在此情况
        kout[red] << "The Process has Not Child Process!" << endl;
        return -1;
    }
    while (1)
    {
        proc_struct* child = nullptr;
        // 利用链接部分的指针关系去寻找满足的孩子进程
        proc_struct* pptr = nullptr;
        for (pptr = cur_proc->fir_child;pptr != nullptr;pptr = pptr->bro_next)
        {
            if (pptr->stat == Proc_finished)
            {
                // 找到对应的进程
                if (pid == -1 || pptr->pid == pid)
                {
                    child = pptr;
                    break;
                }
            }
        }
        if (child != nullptr)
        {
            // 当前的child进程即需要wait的进程
            int ret = child->pid;
            if (status != nullptr)
            {
                // status非空则需要提取相应的状态
                VMS::EnableAccessUser();
                *status = child->exit_value << 8;   // 左移8位主要是Linux系统调用的规范
                VMS::DisableAccessUser();
            }
            pm.free_proc(child);                    // 回收子进程
            return ret;
        }
        else if (options & WNOHANG)
        {
            // 当没有找到符合的子进程但是options中指定了WNOHANG
            // 直接返回 -1即可
            return -1;
        }
        else
        {
            // 说明还需要等待子进程
            pm.calc_systime(cur_proc);              // 从这个时刻之后的wait时间不应被计算到systime中
            cur_proc->sem->wait();                  // 父进程在子进程上等待 当子进程exit后解除信号量等待可以被回收
        }
    }
    return -1;
}

inline int SYS_getppid()
{
    // 获取父进程PID的系统调用
    // 直接成功返回父进程的ID
    // 根据系统调用规范此调用 always successful

    proc_struct* cur_proc = pm.get_cur_proc();
    proc_struct* fa_proc = cur_proc->fa;
    if (fa_proc == nullptr)
    {
        // 调用规范总是成功
        // 如果是无父进程 可以暂时认为将该进程挂到根进程的孩子进程下
        pm.set_fa_proc(cur_proc, idle_proc);
        return 0;
    }
    else
    {
        return fa_proc->pid;
    }
    return -1;
}

inline int SYS_getpid()
{
    // 获取进程PID的系统调用
    // always successful

    proc_struct* cur_proc = pm.get_cur_proc();
    return cur_proc->pid;
}























struct tms
{
    long tms_utime;                 // user time
    long tms_stime;                 // system time
    long tms_cutime;                // user time of children
    long tms_sutime;                // system time of children
};

inline clock_t SYS_times(tms* tms)
{
    // 获取进程时间的系统调用
    // tms结构体指针 用于获取保存当前进程的运行时间的数据
    // 成功返回已经过去的滴答数 即real time
    // 这里的time基本即指进程在running态的时间
    // 系统调用规范指出为花费的CPU时间 故仅考虑运行时间
    // 而对于running态时间的记录进程结构体中在每次从running态切换时都会记录
    // 对于用户态执行的时间不太方便在用户态记录
    // 这里我们可以认为一个用户进程只有在陷入系统调用的时候才相当于在使用核心态的时间
    // 因此我们在每次系统调用前后记录本次系统调用的时间并加上
    // 进而用总的runtime减去这个时间就可以得到用户时间了
    // 记录这个时间的成员在进程结构体中提供为systime

    proc_struct* cur_proc = pm.get_cur_proc();
    clock_t time_unit = timer_1us;  // 填充tms结构体的基本时间单位 在qemu上模拟 以微秒为单位
    if (tms != nullptr)
    {
        clock_t run_time = pm.get_proc_rumtime(cur_proc, 1);    // 总运行时间
        clock_t sys_time = pm.get_proc_systime(cur_proc, 1);    // 用户陷入核心态的system时间
        clock_t user_time = run_time - sys_time;                // 用户在用户态执行的时间
        if ((int64)run_time < 0 || (int64)sys_time < 0 || (int64)user_time < 0)
        {
            // 3个time有一个为负即认为出错调用失败了
            // 返回-1
            return -1;
        }
        cur_proc->vms->EnableAccessUser();                      // 在核心态操作用户态的数据
        tms->tms_utime = user_time / time_unit;
        tms->tms_stime = sys_time / time_unit;
        // 基于父进程执行到这里时已经wait了所有子进程退出并被回收了
        // 故直接置0
        tms->tms_cutime = 0;
        tms->tms_sutime = 0;
        cur_proc->vms->DisableAccessUser();
    }
    return get_clock_time();
}

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

inline int SYS_uname(utsname* uts)
{
    // 打印系统信息的系统调用
    // utsname结构体指针用于获取系统信息数据
    // 成功返回0 失败返回-1
    // 相关信息的字符串处理

    if (uts == nullptr)
    {
        return -1;
    }

    // 操作用户区的数据
    // 需要从核心态进入用户态
    VMS::EnableAccessUser();
    strcpy(uts->sysname, "DBStars_OperatingSystem");
    strcpy(uts->nodename, "DBStars_OperatingSystem");
    strcpy(uts->release, "Debug");
    strcpy(uts->version, "0.7");
    strcpy(uts->machine, "RISCV 64");
    strcpy(uts->domainname, "DBStars");
    VMS::DisableAccessUser();

    return 0;
}

inline int SYS_sched_yield()
{
    // 让出调度器的系统调用
    // 成功返回0 失败返回-1
    // 即pm中的rest功能

    proc_struct* cur_proc = pm.get_cur_proc();
    pm.rest_proc(cur_proc);
    return 0;
}

struct timeval
{
    uint64 sec;                     // 自 Unix 纪元起的秒数
    uint64 usec;                    // 微秒数
};

inline int SYS_gettimeofday(timeval* ts, int tz = 0)
{
    // 获取时间的系统调用
    // timespec结构体用于获取时间值
    // 成功返回0 失败返回-1

    if (ts == nullptr)
    {
        return -1;
    }

    // 其他需要的信息测试平台已经预先软件处理完毕了
    VMS::EnableAccessUser();
    clock_t cur_time = get_clock_time();
    ts->sec = cur_time / timer_1s;
    ts->usec = (cur_time % timer_1s) / timer_1us;
    VMS::DisableAccessUser();
    return 0;
}

struct timespec
{
    long tv_sec;                    // 秒
    long tv_nsec;                   // 纳秒 范围0~999999999
};

inline int SYS_nanosleep(timespec* req, timespec* rem)
{
    // 执行线程睡眠的系统调用
    // sleep()库函数基于此系统调用
    // 输入睡眠的时间间隔
    // 成功返回0 失败返回-1

    if (req == nullptr || rem == nullptr)
    {
        return -1;
    }

    proc_struct* cur_proc = pm.get_cur_proc();
    semaphore.init(0);              // 利用信号量实现sleep 注意区分这里的全局信号量和每个进程内部的信号量的区别
    clock_t wait_time = 0;          // 计算qemu上需要等待的时钟滴答数
    VMS::EnableAccessUser();
    wait_time = req->tv_sec * timer_1s +
        req->tv_nsec / (long)1e6 * timer_1ms +
        req->tv_nsec % (long)1e6 / 1e3 * timer_1us +
        req->tv_nsec % (long)1e3 * timer_1ns;
    rem->tv_sec = rem->tv_nsec = 0;
    VMS::DisableAccessUser();
    semaphore.wait(cur_proc);       // 当前进程在semaphore上等待 切换为sleeping态
    pm.set_waittime_limit(cur_proc, wait_time);
    clock_t start_timebase = get_clock_time();
    while (1)
    {
        clock_t cur_time = get_clock_time();
        if (cur_time - start_timebase >= wait_time)
        {
            semaphore.signal();
            break;
        }
    }
    semaphore.destroy();            // 这个信号量仅在这里使用 每次使用都是初始化 使用完了即释放空间
    return 0;
}









// 系统调用方式遵循RISC-V ABI
// 即调用号存放在a7寄存器中 6个参数分别存放在a0-a5寄存器中 返回值存放在a0寄存器中
void trap_Syscall(TRAPFRAME* tf)
{
    // debug
    if (tf->reg.a7 >= 0)
    {
        // kout[green] << "Process's pid " << pm.get_cur_proc()->pid << " Syscall:" << endl;
        // kout[green] << "Syscall Number " << tf->reg.a7 << endl;
        // kout[green] << "Syscall name " << get_syscall_name(tf->reg.a7) << endl;
    }
    switch (tf->reg.a7)
    {
    case Sys_putchar:
        SYS_putchar(tf->reg.a0);
        break;
    case Sys_getchar:
        tf->reg.a0 = SYS_getchar();
        break;
    case Sys_getcwd:
        break;
    case Sys_pipe2:
        break;
    case Sys_dup:
        break;
    case Sys_dup3:
        break;
    case Sys_chdir:
        break;
    case Sys_openat:
        break;
    case Sys_close:
        break;
    case Sys_getdents64:
        break;
    case Sys_read:
        break;
    case Sys_write:
        break;
    case Sys_linkat:
        break;
    case Sys_unlinkat:
        break;
    case Sys_mkdirat:
        break;
    case Sys_umount2:
        break;
    case Sys_mount:
        break;
    case Sys_fstat:
        break;
    case Sys_clone:
        // kout[green] << tf->reg.a0 << endl;
        tf->reg.a0 = SYS_clone(tf, tf->reg.a0, (void*)tf->reg.a1, tf->reg.a2, tf->reg.a3, tf->reg.a4);
        // kout[green] << tf->reg.a0 << endl;
        // kout[green] << tf->reg.a1 << endl;
        break;
    case Sys_execve:
        break;
    case Sys_wait4:
        tf->reg.a0 = SYS_wait4(tf->reg.a0, (int*)tf->reg.a1, tf->reg.a2);
        break;
    case Sys_exit:
        SYS_exit(tf->reg.a0);
        break;
    case Sys_getppid:
        tf->reg.a0 = SYS_getppid();
        break;
    case Sys_getpid:
        tf->reg.a0 = SYS_getpid();
        break;
    case Sys_brk:
        break;
    case Sys_munmap:
        break;
    case Sys_mmap:
        break;
    case Sys_times:
        tf->reg.a0 = SYS_times((tms*)tf->reg.a0);
        break;
    case Sys_uname:
        tf->reg.a0 = SYS_uname((utsname*)tf->reg.a0);
        break;
    case Sys_sched_yield:
        tf->reg.a0 = SYS_sched_yield();
        break;
    case Sys_gettimeofday:
        tf->reg.a0 = SYS_gettimeofday((timeval*)tf->reg.a0, 0);
        break;
    case Sys_nanosleep:
        tf->reg.a0 = SYS_nanosleep((timespec*)tf->reg.a0, (timespec*)tf->reg.a1);
        break;
    default:
        // 遇到了未定义的系统调用号
        // 这里的处理就是输出相关信息即可
        kout[yellow] << "The Syscall has not defined!" << endl;
        kout[yellow] << "Current Process's pid is " << pm.get_cur_proc()->pid << endl;
        if (pm.get_cur_proc()->ku_flag == 2)
        {
            // 是用户进程发起的系统调用
            // 那么这个用户进程遇到这种情况应该直接终止
            pm.exit_proc(pm.get_cur_proc(), -1);
            kout[red] << "trap_Syscall : reached unreachable branch!" << endl;
        }
        break;
    }
}