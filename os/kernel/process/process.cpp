#include <process.hpp>
#include <kout.hpp>
#include <kstring.hpp>

void ProcessManager::add_proc_tolist(proc_struct* proc)
{
    // 采取最简单的方式 直接加入最后一个位置
    proc_struct* sptr = proc_listhead.next;
    while (sptr->next != nullptr)
    {
        sptr = sptr->next;
    }
    sptr->next = proc;
    proc->next = nullptr;
}

bool ProcessManager::remove_proc_fromlist(proc_struct* proc)
{
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
    proc->pid_fa = -1;
    proc->pid_bro_pre = -1;
    proc->pid_bro_next = -1;
    proc->pid_fir_child = -1;
    memset(&(proc->context), 0, sizeof proc->context);
    memset(proc->name, 0, PROC_NAME_LEN);

    cur_proc++;                     // 分配出的进程数量加1
    return proc;
}

bool ProcessManager::init_proc(proc_struct* proc,int ku_flag)
{
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
        proc->ku_flag = ku_flag;
        proc->pid = tm_pid;
        // ... 其他待定
    }
    else
    {
        add_proc_tolist(proc);
        proc->stat = Proc_ready;
        proc->kstack = nullptr;
        proc->kstacksize = 0;
        proc->ku_flag = ku_flag;
        proc->pid = tm_pid;
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
    }
    return sret;
}

bool ProcessManager::free_proc(proc_struct* proc)
{
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
    if (proc->kstack != nullptr)
    {
        // 调用这个释放所属的未释放的资源
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
    cur_proc--;
    return true;
}

bool ProcessManager::set_proc_name(proc_struct* proc, char* name)
{
    int namelen = strlen(name);
    if (namelen >= PROC_NAME_LEN)
    {
        kout[yellow] << "The Process's name is too long!" << endl;
        return false;
    }
    strcpy(proc->name, name);
    return true;
}

bool ProcessManager::set_proc_kstk(proc_struct* proc, void* kaddr, uint32 size)
{
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
    idle_proc->kstacksize = KERNELSTACKSIZE * 4;    // 默认内核栈为16KB
    char* our_kernel_name = "DBStars_Operating_System";
    set_proc_name(idle_proc, our_kernel_name);      // 设置我们的根进程的名字!
    add_proc_tolist(idle_proc);                     // 别忘了加入进程链表 永驻第1个节点位置
    idle_proc->stat = Proc_running;                 // 初始化完了就让这个进程一直跑起来吧
}

void ProcessManager::Init()
{
    // PM类的初始化主要就是初始化出 idle 0号进程
    init_idleproc_for_boot();
    cur_proc = idle_proc;                           // 别忘了设置cur_proc

    // 输出调试信息
    kout[green] << idle_proc->name << endl;
    kout[green] << "cur_proc : " << cur_proc->name << endl;
    kout[green] << "cur_proc's pid : " << cur_proc->pid << endl;
    kout[green] << "idle_proc's pid : " << idle_proc->pid << endl;
    kout[green] << "ProcessManager Init Successfully!" << endl;
}




ProcessManager pm;