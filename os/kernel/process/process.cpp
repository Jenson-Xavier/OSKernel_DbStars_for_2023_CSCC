#include <process.hpp>
#include <kout.hpp>
#include <kstring.hpp>
#include <Riscv.h>

// 定义idle进程
proc_struct* idle_proc = nullptr;

void ProcessManager::show(proc_struct* proc)
{
    // 调试需要
    kout << "proc pid : " << proc->pid << endl
        << "proc name : " << proc->name << endl
        << "proc state : " << proc->stat << endl
        << "proc kuflag : " << proc->ku_flag << endl
        << "proc kstack : " << Hex((uint64)proc->kstack) << endl
        << "proc kstacksize : " << proc->kstacksize << endl
        << "proc pid_fa : " << proc->pid_fa << endl
        << "proc pid_bro_pre : " << proc->pid_bro_pre << endl
        << "proc pid_bro_next : " << proc->pid_bro_next << endl
        << "proc pid_fir_child : " << proc->pid_fir_child << endl
        << "-------------------------------------------" << endl;
}

void ProcessManager::print_all_list()
{
    proc_struct* sptr = proc_listhead.next;
    while (sptr != nullptr)
    {
        show(sptr);
        sptr = sptr->next;
    }
}

void ProcessManager::add_proc_tolist(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    // 采取最简单的方式 直接加入最后一个位置
    proc_struct* sptr = proc_listhead.next;

    // 需要注意的是 第一个节点应该是idle_proc
    // 因此在0号进程未初始化时这里的sptr为空
    // 手动处理这种情况
    if (sptr == nullptr)
    {
        // 对应添加的节点就是idle_proc进程
        proc_listhead.next = proc;
        proc->next = nullptr;
        return;
    }

    while (sptr->next != nullptr)
    {
        sptr = sptr->next;
    }
    sptr->next = proc;
    proc->next = nullptr;
}

bool ProcessManager::remove_proc_fromlist(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // 进程链表第0个进程(非头节点)是boot idle进程 不会考虑它的移除
    // 直接作为头节点指针 进程链表节点的删除
    proc_struct* spre = proc_listhead.next;
    if (spre == nullptr || spre->next == nullptr)
    {
        // 说明没有移除节点的需要
        kout[yellow] << "Nothing node to be Removed!" << endl;
        return false;
    }

    proc_struct* scur = spre->next;
    while (scur != nullptr)
    {
        if (scur == proc)
        {
            spre->next = scur->next;
            kfree(scur);
            break;
        }
        scur = scur->next;
        spre = spre->next;
    }
    return true;
}

int ProcessManager::get_unique_pid()
{
    int ret_pid = -1;
    bool st[MAX_PID + 1];
    memset(st, 0, sizeof st);
    proc_struct* sptr = proc_listhead.next;
    while (sptr != nullptr)
    {
        if (sptr->pid >= 0)
        {
            st[sptr->pid] = 1;
        }
        sptr = sptr->next;
    }
    for (int i = 1;i <= MAX_PID;i++)
    {
        // 直接从1号开始找
        // 默认0号pid是分配给第0个idle boot进程
        if (st[i] == 0)
        {
            ret_pid = i;
            break;
        }
    }
    return ret_pid;
}

proc_struct* ProcessManager::alloc_proc()
{
    struct proc_struct* proc = (proc_struct*)kmalloc(sizeof(struct proc_struct));
    if (proc == nullptr)
    {
        kout[red] << "Alloc Process Fail!" << endl;
        return nullptr;
    }
    proc->next = nullptr;
    proc->stat = Proc_allocated;
    proc->pid = -1;                 // -1表示还不是一个正确的进程
    proc->ku_flag = 0;              // 表示未初始化 既不是内核又不是用户
    proc->kstack = nullptr;
    proc->kstacksize = 0;
    proc->context = nullptr;
    proc->pid_fa = -1;
    proc->pid_bro_pre = -1;
    proc->pid_bro_next = -1;
    proc->pid_fir_child = -1;
    memset(&(proc->context), 0, sizeof proc->context);
    memset(proc->name, 0, PROC_NAME_LEN);

    proc_count++;                   // 分配出的进程数量加1
    return proc;
}

bool ProcessManager::init_proc(proc_struct* proc, int ku_flag)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // 说明当前进程已经修改过 不应该再初始化
    if (proc->ku_flag != 0 || proc->stat != Proc_allocated)
    {
        kout[yellow] << "The Process has Inited!" << endl;
        return false;
    }

    /*
        初始化进程 主要是以下这些方面
        1. 将进程加入调度器的进程链表
        2. 改变进程的状态
        3. 指定默认的内核栈空间为0 (这个可以手动指定分配)
        4. 改变对应的内核/用户标志
        5. 其他的部分需要在其他场景下根据需要改变
        6. 分配一个独特的进程号
        7. 设置时间相关信息
    */

    int tm_pid = get_unique_pid();
    if (tm_pid == -1)
    {
        kout[red] << "Get pid Fail and Cannot Init a Process!" << endl;
        return false;
    }

    // 初始化进程 ku_flag为1表示内核级
    // 为2表示用户级别 暂时用户级数据还没有维护好 不去设计
    if (ku_flag == 1)
    {
        add_proc_tolist(proc);
        proc->stat = Proc_ready;
        proc->kstack = nullptr;     //这个在分配过程已经完成
        proc->kstacksize = 0;
        proc->context = nullptr;
        proc->ku_flag = ku_flag;
        proc->pid = tm_pid;
        proc->timebase = get_clock_time();
        proc->runtime = 0;
        proc->sleeptime = 0;
        proc->readytime = 0;
        // ... 其他待定
    }
    else
    {
        add_proc_tolist(proc);
        proc->stat = Proc_ready;
        proc->kstack = nullptr;
        proc->kstacksize = 0;
        proc->context = nullptr;
        proc->ku_flag = ku_flag;
        proc->pid = tm_pid;
        proc->timebase = get_clock_time();
        proc->runtime = 0;
        proc->sleeptime = 0;
        proc->readytime = 0;
        // ... 其他待定
    }
    return true;
}

proc_struct* ProcessManager::get_proc(int pid)
{
    // 从进程链表头节点出发寻找对应pid
    proc_struct* sptr = nullptr;
    proc_struct* sret = nullptr;
    sptr = proc_listhead.next;
    while (sptr != nullptr)
    {
        if (sptr->pid == pid)
        {
            sret = sptr;
            break;
        }
        sptr = sptr->next;
    }
    return sret;
}

bool ProcessManager::free_proc(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // 这个封装主要执行的过程就是remove_proc_fromlist过程
    // 但是考虑到最终实现每个结构体还有很多其他分配的空间
    // 因此这一步就是执行一个安全检查并且确认对应的资源释放完毕后移除

    if (proc->pid == 0)
    {
        //0号的根进程不应该被free
        kout[red] << "Zero Root Idle Process Should Not be Free!" << endl;
        return false;
    }

    // 当然目前主要只有一个内核栈空间需要检查
    int check_ret = 0;
    if (proc->kstack != nullptr || proc->kstacksize > 0)
    {
        // 调用这个释放所属的未释放的资源
        // kout[yellow] << "The process has resources to be finished!" << endl;
        check_ret = finish_proc(proc);
        if (check_ret != 0)
        {
            // 如果设计了对应的错误码 ...
            return false;
        }
    }
    else
    {
        // ... 后续还有文件系统 内存管理 IPC等等
    }

    // 上述确保进程相关的所属资源已经完全释放
    // 可以放心地从进程链表中remove
    remove_proc_fromlist(proc);
    proc_count--;
    return true;
}

int ProcessManager::finish_proc(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // finish_proc函数的作用主要是释放进程结构所属的资源
    // 而不是释放这个结构本身 这个任务在free_proc里面完成
    // 因该遵循这样的内存释放逻辑

    // 目前主要只有一个内核栈需要检查
    if (proc->kstacksize > 0 || proc->kstack != nullptr)
    {
        kfree(proc->kstack);
        proc->kstack = nullptr;
        proc->kstacksize = 0;
    }

    // ... 虚存 文件 IPC等其他部分

    return 0;
}

bool ProcessManager::set_proc_name(proc_struct* proc, char* name)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    memset(proc->name, 0, sizeof proc->name);
    int namelen = strlen(name);
    if (namelen >= PROC_NAME_LEN)
    {
        kout[yellow] << "The Process's name is too long!" << endl;
        return false;
    }
    memcpy(proc->name, name, PROC_NAME_LEN);
    return true;
}

bool ProcessManager::set_proc_kstk(proc_struct* proc, void* kaddr, uint32 size)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    if (proc->kstacksize != 0)
    {
        kout[yellow] << "The Process's kernelstack has allocated!" << endl;
        return false;
    }

    if (kaddr == nullptr)
    {
        // 指定的内核栈地址为nullptr 说明分配地址按默认的kmalloc进行
        // 一般不会用到指定kaddr参数的情况
        kaddr = kmalloc((uint64)size);
        if (kaddr == nullptr)
        {
            kout[red] << "The Process's kernelstack Alloc Fail!" << endl;
            return false;
        }
    }
    proc->kstack = kaddr;
    proc->kstacksize = size;
    return true;
}

void ProcessManager::init_idleproc_for_boot()
{
    // 初始化0号boot进程 也可以认为是根进程
    // 相当于"继承"了我们的OS内核的运行
    idle_proc = alloc_proc();           // 给boot进程分配空间
    if (idle_proc == nullptr)
    {
        // idle进程不允许分配失败 这个分支不应该出现
        kout[red] << "Idle Boot Process Alloc Fail!" << endl;
        return;
    }

    // 进程初始化 这个idle进程比较特殊 不使用初始化函数进行
    // 手动初始化
    idle_proc->ku_flag = 1;                         // 内核级进程
    idle_proc->stat = Proc_ready;                   // 初始化状态为就绪态
    idle_proc->pid = 0;                             // 默认分配0号pid 特质idle进程
    idle_proc->pid_fa = -1;                         // 根进程 永远不会存在父进程
    idle_proc->kstack = boot_stack;                 // 第0个进程 内核栈地址直接放在boot_stack位置
    idle_proc->kstacksize = boot_stack_top - boot_stack;
    set_proc_name(idle_proc, (char*)"DBStars_Operating_System");
    add_proc_tolist(idle_proc);                     // 别忘了加入进程链表 永驻第1个节点位置
    pm.switchstat_proc(idle_proc, Proc_running);                 // 初始化完了就让这个进程一直跑起来吧
}

void ProcessManager::Init()
{
    // 初始化相关变量
    proc_listhead.pid = -1;                         // dummyhead 进程号永远为-1
    cur_proc = nullptr;

    // PM类的初始化主要就是初始化出 idle 0号进程
    init_idleproc_for_boot();
    cur_proc = idle_proc;                           // 别忘了设置cur_proc

    // 输出相关信息
    kout[green] << "ProcessManager_init Success!" << endl;
    kout[green] << "Current Process : " << cur_proc->name << endl;
    kout << endl;
}

bool ProcessManager:: switchstat_proc(proc_struct* proc, uint32 tar_stat)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    clock_t curtime = get_clock_time();
    clock_t duration = curtime - proc->timebase;
    proc->timebase = curtime;
    switch (proc->stat)
    {
    case Proc_allocated:
        if (tar_stat != Proc_ready)
        {
            break;                      // Proc_allocated只能进入Proc_ready
        }
        kout[yellow] << "Switch the Process should init it!" << endl;
        return false;
    case Proc_ready:
        if (tar_stat == Proc_allocated)
        {
            break;
        }
        proc->readytime += duration;
        proc->stat = tar_stat;
        break;
    case Proc_running:
        if (tar_stat == Proc_allocated)
        {
            break;
        }
        proc->runtime += duration;
        proc->stat = tar_stat;
        break;
    case Proc_sleeping:
        if (tar_stat == Proc_allocated)
        {
            break;
        }
        proc->sleeptime += duration;
        proc->stat = tar_stat;
        break;
    case Proc_finished:
        break;
    case Proc_zombie:
        break;
    }
    return true;
}

bool ProcessManager::start_kernel_proc(proc_struct* proc, int (*func)(void*), void* arg)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    if (proc->kstack == nullptr && proc->kstacksize == 0)
    {
        kout[red] << "The Kernel Process doesn't have KernelStack!" << endl;
        return false;
    }

    // 在内核栈上开辟一块trapframe来保存进程上下文
    proc->context = (TRAPFRAME*)((char*)proc->kstack + proc->kstacksize) - 1;
    uint64 SPP = 1 << 8;            // 利用中断 设置sstatus寄存器 Supervisor Previous Privilege(guess)
    uint64 SPIE = 1 << 5;           // Supervisor Previous Interrupt Enable
    uint64 SIE = 1 << 1;            // Supervisor Interrupt Enable
    proc->context->status = (read_csr(sstatus) | SPP | SPIE) & (~SIE);      // 详见手册
    proc->context->epc = (uint64)KernelProcessEntry;                        // Exception PC
    proc->context->reg.s0 = (uint64)func;
    proc->context->reg.s1 = (uint64)arg;
    proc->context->reg.s2 = (uint64)proc;
    proc->context->reg.sp = (uint64)((char*)proc->kstack + proc->kstacksize);
    pm.switchstat_proc(proc, Proc_ready);
    return true;
    
    // 这里通过注释补充一下这里的上下文实现逻辑 因为这一块的设计还是比较具有独创性的 相当绕
    // 每个进程的context成员其实更规范可以叫做 last_context
    // 仔细联系调度器scheduler的代码逻辑
    // 其实是打破了正常的trap上下文设置的逻辑
    // 在scheduler中 传入的参数是当前的TRAPFRAME 将它重新赋给cur_proc
    // 这一步其实是实现了所说的"保存上下文"的操作 对应到具体进程上去
    // 返回值就是需要调度的进程的上下文结构 也就是这里的start函数的设置逻辑
    // 在trap入口处理完调度器的逻辑后返回值在trap入口的汇编代码会看到被重新放到sp处
    // 之后的restore其实恢复的寄存器是返回值的TRAPFRAME结构
    // 因而执行完sret后跳转到epc对应的pc处恢复之前的执行流就自然跳转到当前要调度的进程上去了
    // 这里的返回值并不是trap之间通过STORE指令保存在栈上的那些值了
    // 当然如果只是单纯时钟中断 返回的TRAPFRAME和传入的参数一致 也就是正常上下文保存和恢复流程
    // 但是这里并没有任何影响 
    // 因为需要恢复的原先保存的那一段上下文TRAMFRAME结构已经在代码中放到所属进程结构体的对应成员中去了
}

// 内核进程的退出
extern "C"
{
    void KernelProcessExit(proc_struct* proc)
    {
        pm.switchstat_proc(proc, Proc_finished);        // 进程运行结束 退出的状态设置
        asm volatile("li a7,1; ebreak");                // 内联汇编 设置a7寄存器 通过ebreak触发断点异常进行进程回收
        kout[yellow] << "This Block should not occur!" << endl;
    }
}

TRAPFRAME* ProcessManager::proc_scheduler(TRAPFRAME* context)
{
    // 进程调度器的核心功能 利用trap进行进程调度
    
    if (cur_proc == nullptr)
    {
        kout[red] << "The Current Process is Nullptr!" << endl;
    }
    
    cur_proc->context = context;                    // 相当于进行的上下文的保存工作
    proc_struct* sptr = nullptr;
    sptr = cur_proc;
    // 暂时就是遍历进程链表去寻找第一个可调度的进程 测试逻辑
    while (sptr != nullptr)
    {
        if (sptr->stat == Proc_ready)
        {
            if (cur_proc->stat == Proc_running)
            {
                pm.switchstat_proc(cur_proc, Proc_ready);
            }
            cur_proc = sptr;
            pm.switchstat_proc(cur_proc, Proc_running);
            return cur_proc->context;
        }
        else
        {
            if (sptr->stat == Proc_finished || sptr->stat == Proc_zombie)
            {
                proc_struct* freed = sptr;
                sptr = sptr->next;
                pm.free_proc(freed);
            }
            else
            {
                sptr = sptr->next;
            }
        }
    }
    
    // 没有return 将前一半继续遍历一次
    sptr = proc_listhead.next;
    while (sptr != cur_proc)
    {
        if (sptr->stat == Proc_ready)
        {
            if (cur_proc->stat == Proc_running)
            {
                pm.switchstat_proc(cur_proc, Proc_ready);
            }
            cur_proc = sptr;
            pm.switchstat_proc(cur_proc, Proc_running);
            return cur_proc->context;
        }
        else
        {
            if (sptr->stat == Proc_finished || sptr->stat == Proc_zombie)
            {
                proc_struct* freed = sptr;
                sptr = sptr->next;
                pm.free_proc(freed);
            }
            else
            {
                sptr = sptr->next;
            }
        }
    }

    if (cur_proc != nullptr && cur_proc->stat == Proc_running)
    {
        // 继续执行当前的进程流
        return cur_proc->context;
    }

    // 还没有返回 就是没有可调度进程
    kout[red] << "No Process to Schedule!" << endl;
    return nullptr;
}

proc_struct* CreateKernelProcess(int (*func)(void*), void* arg, char* name)
{
    proc_struct* k_proc = pm.alloc_proc();
    pm.init_proc(k_proc, 1);
    pm.set_proc_name(k_proc, name);
    pm.set_proc_kstk(k_proc, nullptr, KERNELSTACKSIZE * 4);
    // VMS...
    pm.start_kernel_proc(k_proc, func, arg);
    return k_proc;
}

ProcessManager pm;