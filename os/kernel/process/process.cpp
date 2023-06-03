#include <process.hpp>
#include <kout.hpp>
#include <kstring.hpp>
#include <Riscv.h>
#include <interrupt.hpp>
#include <semaphore.hpp>
#include <fileobject.hpp>

// 定义idle进程
proc_struct* idle_proc = nullptr;

// 定义初始化立即调度与否的变量
bool imme_schedule = false;

void ProcessManager::show(proc_struct* proc)
{
    // 调试需要
    kout << "proc pid : " << proc->pid << endl
        << "proc name : " << proc->name << endl
        << "proc state : " << proc->stat << endl
        << "proc kuflag : " << proc->ku_flag << endl
        << "proc kstack : " << Hex((uint64)proc->kstack) << endl
        << "proc kstacksize : " << proc->kstacksize << endl
        << "proc pid_fa : " << proc->fa << endl
        << "proc pid_bro_pre : " << proc->bro_pre << endl
        << "proc pid_bro_next : " << proc->bro_next << endl
        << "proc pid_fir_child : " << proc->fir_child << endl
        << "proc vms addr : " << Hex((uint64)proc->vms) << endl
        << "proc vms : " << endl;
    proc->vms->show();
    kout << "-------------------------------------------" << endl;
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
    proc->trap_sys_timebase = 0;
    proc->waittime_limit = 0;       // 这个只有在相关场景下会手动设置 无需初始化
    proc->context = nullptr;
    proc->vms = nullptr;
    proc->heap = nullptr;           // heap属性事实上只是为了brk系统调用的需要 在解析elf和系统调用实现时会进行更新 进程管理部分不会修改
    proc->fa = nullptr;
    proc->bro_pre = nullptr;
    proc->bro_next = nullptr;
    proc->fir_child = nullptr;
    proc->exit_value = 0;           // 只有在异常退出时才需要设置 大部分情况下均为0
    proc->flags = 0;                // 系统调用时需要 其他场景暂时不会使用
    proc->sem = nullptr;
    proc->fo_head = nullptr;        // 文件对象 即常说的文件描述符表指针
    proc->cur_work_dir = nullptr;   // 类似heap属性 在进程管理初始化和创建部分不会觉得它的值 elf解析时会有相关设置
    memset(&(proc->context), 0, sizeof proc->context);
    memset(proc->name, 0, PROC_NAME_LEN);

    proc_count++;                   // 分配出的进程数量加1
    return proc;
}

bool ProcessManager::init_proc(proc_struct* proc, int ku_flag, int flags)
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
        8. 初始化虚存管理相关信息
        9. 分配并初始化信号量信息
       10. 分配并初始化fd表的头节点
    */

    int tm_pid = get_unique_pid();
    if (tm_pid == -1)
    {
        kout[red] << "Get pid Fail and Cannot Init a Process!" << endl;
        return false;
    }

    // 初始化进程
    // ku_flag为1表示内核级
    // 为2表示用户级别
    if (ku_flag == 1)
    {
        add_proc_tolist(proc);
        proc->stat = Proc_ready;
        proc->kstack = nullptr;     // 这个在分配过程已经完成
        proc->kstacksize = 0;
        proc->context = nullptr;
        proc->ku_flag = ku_flag;
        proc->pid = tm_pid;
        proc->timebase = get_clock_time();
        proc->runtime = 0;
        proc->systime = 0;          // 对于内核进程可以认为不需要使用这个成员变量     
        proc->sleeptime = 0;
        proc->readytime = 0;
        proc->vms = nullptr;
        proc->flags = flags;
        proc->sem = (SEMAPHORE*)kmalloc(sizeof(SEMAPHORE));
        proc->sem->init();          // 初始化每个进程的信号量值设为0 wait系统调用的需要
        fom.init_proc_fo_head(proc);
        pm.init_proc_fds(proc);
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
        proc->systime = 0;          // 虽然此时也在核心态 但是我们只考虑运行时间
        proc->runtime = 0;
        proc->sleeptime = 0;
        proc->readytime = 0;
        proc->vms = nullptr;
        proc->flags = flags;
        proc->sem = (SEMAPHORE*)kmalloc(sizeof(SEMAPHORE));
        proc->sem->init();
        fom.init_proc_fo_head(proc);
        pm.init_proc_fds(proc);
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

    // 在检查资源的释放之前
    // 需要先检查当前需要被释放的进程是否有子进程
    // 如果存在子进程 让它们都成为孤儿进程
    // 进而调度器逻辑保证能够回收它们
    // 不过在正确的执行逻辑下子进程一定会先被父进程回收 不会执行到这里
    proc_struct* child = nullptr;
    for (child = cur_proc->fir_child;child != nullptr;child = child->bro_next)
    {
        child->fa = nullptr;
    }

    // 整个进程管理部分free进程时需要检查的资源
    // 基本类型和指针本身类型所占空间在free结构体时会被释放
    // 重点时检查指针指向的那一块空间的资源
    // 内核栈空间需要检查
    // 虚存管理分配空间需要检查
    // 信号量资源
    // heap指针的资源
    // 文件描述符链表的资源
    // cwd字符指针占用的资源
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
    if (proc->vms != nullptr)
    {
        // 当然如果前面某个if就调用了这个函数 那么应该所有需要被检查释放的资源都已经释放
        check_ret = finish_proc(proc);
        if (check_ret != 0)
        {
            return false;
        }
    }
    if (proc->sem != nullptr)
    {
        check_ret = finish_proc(proc);
        if (check_ret != 0)
        {
            return false;
        }
    }
    if (proc->heap != nullptr)
    {
        check_ret = finish_proc(proc);
        if (check_ret != 0)
        {
            return false;
        }
    }
    if (proc->fo_head != nullptr)
    {
        check_ret = finish_proc(proc);
        if (check_ret != 0)
        {
            return false;
        }
    }
    if (proc->cur_work_dir != nullptr)
    {
        check_ret = finish_proc(proc);
        if (check_ret != 0)
        {
            return false;
        }
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

    // 内核栈需要检查
    // 虚存管理需要检查
    // 信号量以及其上的进程队列需要检查
    // heap堆段指针分配的空间需要检查
    // 文件描述符链表所占用的空间
    // CWD字符指针所占的资源
    if (proc->kstacksize > 0 || proc->kstack != nullptr)
    {
        kfree(proc->kstack);
        proc->kstack = nullptr;
        proc->kstacksize = 0;
    }

    if (proc->vms != nullptr)
    {
        if (proc->vms->GetShareCount() > 1)
        {
            // 这个vms还被其他进程引用(共享内存等)
            // 暂时还不能调用释放资源的函数
            // 这里就解除引用 退出地址空间
            // 并将指针指向置空
            proc->vms->unref();
            proc->vms->Leave();
            proc->vms = nullptr;
        }
        else
        {
            // 可以调用destroy函数清空并释放相关资源了
            proc->vms->Leave();
            proc->vms->destroy();
            proc->vms = nullptr;
        }
    }

    if (proc->sem != nullptr)
    {
        // 对于每一个进程在创建并初始化它时信号量就被分配空间并且被初始化了
        // 因此这里是一定会执行到并且需要释放
        // 先释放信号量上的进程队列的空间
        if (proc->sem->destroy())
        {
            // 只有队列空间被成功释放才可以
            kfree(proc->sem);
        }
        else
        {
            kout[red] << "The Semaphore's queue is not Empty So the sem pointer cannot be freed!" << endl;
            return -1;
        }
    }

    if (proc->heap != nullptr)
    {
        // 这里从设计角度和含义出发只需要释放分配heap指针的资源即可
        kfree(proc->heap);
        proc->heap = nullptr;
    }

    if (proc->fo_head != nullptr)
    {
        // 进程只管理这一块 其继续指向的文件系统相关的资源进程并不管理
        // 这一层封装的绝妙之处
        // 代码管理和层次逻辑上都显出极大好处
        // 先释放链表上除了虚拟头节点的具体的fd表项的资源
        // 最后释放进程体中虚拟头节点的资源
        pm.destroy_proc_fds(proc);
        kfree(proc->fo_head);
        proc->fo_head = nullptr;
    }

    if (proc->cur_work_dir != nullptr)
    {
        kfree(proc->cur_work_dir);
    }

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

void ProcessManager::user_proc_help_init(proc_struct* dst, proc_struct* src)
{
    // 将某个进程控制块的相关信息的迁移和相关初始化工作
    // 这里实现的基础就是旧进程的信息迁移完成后会被回收这个特性
    // 主要是为了实现从特定函数启动用户进程实现
    // 这个场景的copy资源比较特殊 仅需要改变指针的指向即可(本质上还是一个进程)

    // 手动初始化
    // 由于是为了看起来就是一个进程的切换
    // 这里的初始化工作也是手动完成
    // 主要只需要把ku标志和pid手动更改
    add_proc_tolist(dst);
    dst->stat = Proc_ready;
    dst->context = nullptr;
    dst->ku_flag = src->ku_flag;
    dst->pid = src->pid;                // 拷贝辅助进程的pid 从而看起来就像是原来的进程实现了从特定函数的启动并且进入用户态执行用户程序部分的代码
    dst->exit_value = src->exit_value;
    dst->flags = src->flags;
    dst->sem = src->sem;
    src->sem = nullptr;                 // 手动初始化做迁移 将原进程的分配的sem指针给新进程并置空 防止回收时报错
    dst->heap = src->heap;
    src->heap = nullptr;

    // 时间信息的拷贝
    dst->timebase = src->timebase;
    dst->runtime = src->runtime;
    dst->systime = src->systime;
    dst->waittime_limit = src->waittime_limit;

    // 内核栈的初始化
    // 每个进程都需要分配一块内核栈区域
    pm.set_proc_kstk(dst, nullptr, KERNELSTACKSIZE * 4);

    // 链接信息的拷贝
    if (src->fa != nullptr)
    {
        dst->fa = src->fa;
    }
    if (src->bro_pre != nullptr)
    {
        dst->bro_pre = src->bro_pre;
    }
    if (src->bro_next != nullptr)
    {
        dst->bro_next = src->bro_next;
    }
    if (src->fir_child != nullptr)
    {
        dst->fir_child = src->fir_child;
    }

    // vms的拷贝
    // 这里的拷贝就是新旧迁移 为了从特定函数启动的用户进程设置的
    // 而不是利用旧进程的信息创建一个新进程 两者各自的vms独立
    // 故可以利用引用并指向同一个vms地址
    dst->vms = src->vms;
    src->vms = nullptr;

    // 由于是新旧迁移
    // 名字也是一样的
    if (!strcmp(src->name, (char*)""))
    {
        pm.set_proc_name(dst, src->name);
    }

    // 文件系统相关
    // 同样的逻辑
    dst->fo_head = src->fo_head;
    src->fo_head = nullptr;
    dst->cur_work_dir = src->cur_work_dir;
    src->cur_work_dir = nullptr;
}

void ProcessManager::copy_other_proc(proc_struct* dst, proc_struct* src)
{
    // 这里的copy部分与上面的迁移区别就在于是否初始化
    // 并且迁移基于本质上是同一个进程 而拷贝是两个独立的进程
    // 考虑fork和clone调用的需要 不过vms各自处理 故这里不处理
    // 计时时间部分
    dst->timebase = src->timebase;
    dst->runtime = src->runtime;
    dst->systime = src->systime;
    dst->waittime_limit = src->waittime_limit;
    dst->readytime = src->readytime;                // 保留但暂未使用
    dst->sleeptime = src->sleeptime;                // 保留但暂未使用

    // 链接部分
    // 这里是两个现实存在的进程间的拷贝
    // 只需要考虑父进程间的copy
    if (src->fa != nullptr)
    {
        dst->fa = src->fa;
    }

    // 标志位信息 
    dst->flags = src->flags;

    // 进程的名字
    if (!strcmp(src->name, (char*)""))
    {
        pm.set_proc_name(dst, src->name);
    }

    // 文件系统相关
    pm.copy_proc_fds(dst, src);
}

bool ProcessManager::set_proc_vms(proc_struct* proc, VMS* vms)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    if (proc->vms != nullptr)
    {
        // 已经存在了vms 不应该再设置了
        kout[yellow] << "The Process's vms has been set!" << endl;
        return false;
    }

    if (vms == nullptr)
    {
        kout[yellow] << "The vms is Empty cannot be set!" << endl;
        return false;
    }

    // 这个函数的逻辑是直接在外面根据条件分配好一个VMS直接拷贝过去
    // 因此里头的逻辑就是拷贝+增加引用计数
    proc->vms = vms;
    proc->vms->ref();
    return true;
}

bool ProcessManager::set_proc_heap(proc_struct* proc, HMR* hmr)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    if (proc->heap != nullptr)
    {
        // 当前进程的heap已经被设置过 不应该再设置
        // 不考虑覆盖
        kout[yellow] << "The Process's Heap has been set!" << endl;
        return false;
    }

    if (hmr == nullptr)
    {
        kout[yellow] << "The hmr is Empty cannot be set!" << endl;
        return false;
    }

    // 每个hmr是独属于每个用户进程的
    proc->heap = hmr;
    return true;
}

bool ProcessManager::set_proc_fa(proc_struct* proc, proc_struct* fa_proc)
{
    if (proc == nullptr || fa_proc == nullptr)
    {
        kout[red] << "The Process or Father Process has not existed!" << endl;
        return false;
    }

    if (proc->fa != nullptr)
    {
        // 当需要设置的进程已经有一个非空的父进程了
        // 需要做好关系的转移和清理工作
        if (proc->fa == fa_proc)
        {
            // 父子进程和参数中的已经一致
            return true;
        }
        // 关键就是做好关系的转移
        if (proc->fa->fir_child == proc)
        {
            proc->fa->fir_child = proc->bro_next;
        }
        else if (proc->bro_pre != nullptr)
        {
            proc->bro_pre->next = proc->bro_next;
        }
        // 再判断一下bro_next进程
        if (proc->bro_next != nullptr)
        {
            proc->bro_next->bro_pre = proc->bro_pre;
        }
        // 关系转移好了之后清理一下"族谱"关系
        // 一个进程不应该同时具有多个父进程
        proc->bro_pre = nullptr;
        proc->bro_next = nullptr;
        proc->fa = nullptr;
    }

    // 设置新的父进程
    proc->fa = fa_proc;
    proc->bro_next = proc->fa->fir_child;           // 关系的接替 "顶"上去 类头插
    proc->fa->fir_child = proc;
    if (proc->bro_next != nullptr)
    {
        proc->bro_next->bro_pre = proc;
    }
    return true;
}

bool ProcessManager::set_proc_cwd(proc_struct* proc, const char* cwd_path)
{
    // 重新设置进程的当前工作目录
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    if (cwd_path == nullptr)
    {
        kout[red] << "The cwd_path to be set is NULL!" << endl;
        return false;
    }

    if (proc->cur_work_dir != nullptr)
    {
        // 已经存在了则直接释放字符串分配的资源
        kfree(proc->cur_work_dir);
    }
    // 由于是字符指针
    // 不能忘记分配空间
    proc->cur_work_dir = (char*)kmalloc(strlen(cwd_path) + 5);
    strcpy(proc->cur_work_dir, cwd_path);
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
    idle_proc->fa = nullptr;                        // 根进程 永远不会存在父进程
    idle_proc->kstack = boot_stack;                 // 第0个进程 内核栈地址直接放在boot_stack位置
    idle_proc->kstacksize = boot_stack_top - boot_stack;
    idle_proc->timebase = get_clock_time();
    idle_proc->flags = 0;
    set_proc_name(idle_proc, (char*)"DBStars_OperatingSystem");
    add_proc_tolist(idle_proc);                     // 别忘了加入进程链表 永驻第1个节点位置
    set_proc_vms(idle_proc, VMS::GetKernelVMS());   // 0号boot进程的vms地址空间的设置
    idle_proc->sem = (SEMAPHORE*)kmalloc(sizeof(SEMAPHORE));
    idle_proc->heap = nullptr;
    idle_proc->sem->init();                         // 信号量的初始化
    pm.set_proc_cwd(idle_proc, "/");
    fom.init_proc_fo_head(idle_proc);
    pm.init_proc_fds(idle_proc);                    // 进程文件描述符链表初始化
    pm.switchstat_proc(idle_proc, Proc_running);    // 初始化完了就让这个进程一直跑起来吧
}

void ProcessManager::Init()
{
    // 初始化相关变量
    proc_listhead.pid = -1;                         // dummyhead 进程号永远为-1
    cur_proc = nullptr;
    need_imme_run_proc = nullptr;
    need_rest = false;

    // PM类的初始化主要就是初始化出 idle 0号进程
    init_idleproc_for_boot();
    cur_proc = idle_proc;                           // 别忘了设置cur_proc

    // 输出相关信息
    kout[green] << "ProcessManager_init Success!" << endl;
    kout[green] << "Current Process : " << cur_proc->name << endl;
    kout << endl;
}

bool ProcessManager::switchstat_proc(proc_struct* proc, uint32 tar_stat)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // 每次切换状态时更新相关的时间
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
        if (tar_stat != Proc_sleeping)
        {
            // 自唤醒设计的灵活封装与自动化
            // 当从sleeping态唤醒时
            // sleeptime的计时即可置零了
            proc->sleeptime = 0;
        }
        break;
    case Proc_finished:
        break;
    case Proc_zombie:
        break;
    }
    return true;
}

clock_t ProcessManager::get_proc_rumtime(proc_struct* proc, bool need_update)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return 0;
    }

    if (need_update)
    {
        // 说明需要更新
        // 从run态切换回run态即可
        // 利用状态切换封装的函数实现
        if (proc->stat == Proc_running)
        {
            pm.switchstat_proc(proc, Proc_running);
        }
    }
    return proc->runtime;
}

clock_t ProcessManager::get_proc_systime(proc_struct* proc, bool need_update)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return 0;
    }

    if (need_update)
    {
        // 说明需要更新
        // 利用陷入系统调用的计时起点即可
        // 一般不会这样使用
        clock_t cur_time = get_clock_time();
        if (cur_time >= proc->trap_sys_timebase)
        {
            proc->systime += cur_time - proc->trap_sys_timebase;
        }
    }
    return proc->systime;
}

void ProcessManager::set_systiembase(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    // 这个函数只会在进程陷入系统调用时执行
    // 默认参数为当前进程
    proc->trap_sys_timebase = get_clock_time();
}

void ProcessManager::calc_systime(proc_struct* proc, bool is_wait)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    if (is_wait)
    {
        // 如果是wait系统调用
        // systime的核算已经被单独处理了
        // 这里不需要再进行计算
        return;
    }
    clock_t cur_time = get_clock_time();
    if (cur_time >= proc->trap_sys_timebase)
    {
        if (proc->pid == 1)
        {
            // kout[red] << cur_time << endl;
            // kout[blue] << proc->trap_sys_timebase << endl;
            // kout[red] << cur_time - proc->trap_sys_timebase << endl;
        }
        proc->systime += cur_time - proc->trap_sys_timebase;
    }
    proc->trap_sys_timebase = 0;
}

void ProcessManager::set_waittime_limit(proc_struct* proc, clock_t waittime_limit)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    proc->waittime_limit = waittime_limit;
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
    // 这里的返回值并不是trap之前通过STORE指令保存在栈上的那些值了
    // 当然如果只是单纯时钟中断 返回的TRAPFRAME和传入的参数一致 也就是正常上下文保存和恢复流程
    // 但是这里并没有任何影响 
    // 因为需要恢复的原先保存的那一段上下文TRAMFRAME结构已经在代码中放到所属进程结构体的对应成员中去了
}

bool ProcessManager::start_user_proc(proc_struct* proc, int (*func)(void*), void* arg, uint64 user_start_addr)
{
    // 这个函数是为了辅助实现在解析ELF文件装载进程的过程中
    // 装载用户进程时需要让用户进程通过某个特定的函数去启动而实现的
    // 这里采取的实现方式为先是让这个用户进程像内核进程一样执行这个装载函数
    // 当这个函数装载完成之后再像用户进程一样返回到用户态进行执行
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

    proc->context = (TRAPFRAME*)((char*)proc->kstack + proc->kstacksize) - 1;
    uint64 SPP = 1 << 8;
    uint64 SPIE = 1 << 5;
    uint64 SIE = 1 << 1;
    proc->context->status = (read_csr(sstatus) | SPP | SPIE) & (~SIE);
    proc->context->epc = (uint64)UserProcessEntry;
    proc->context->reg.s0 = (uint64)func;
    proc->context->reg.s1 = (uint64)arg;
    proc->context->reg.s2 = (uint64)proc;
    proc->context->reg.s3 = (uint64)user_start_addr;        // 将需要转换并跳转的用户地址也保存下来去传参
    proc->context->reg.sp = (uint64)((char*)proc->kstack + proc->kstacksize);
    pm.switchstat_proc(proc, Proc_ready);
    return true;
}

void ProcessManager::switch_kernel_to_user(proc_struct* proc, uint64 user_start_addr)
{
    // 理论上执行到这个函数不会出现这些需要判断的特殊情况
    // 出于代码的统一和严谨性
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    if (proc->kstack == nullptr && proc->kstacksize == 0)
    {
        kout[red] << "The Kernel Process doesn't have KernelStack!" << endl;
        return;
    }

    // 实现从特定函数启动的用户进程事实上是预创建一个辅助内核进程
    // 它暂时代替执行需要执行的启动函数
    // 为了不改变调度器的逻辑 再创建一个新的用户进程去接替它的信息和使命
    // 事实上这里还是执行一次进程调度
    // 为了统一性我们区分开来并且使用ebreak去实现调度
    proc_struct* u_proc = pm.alloc_proc();
    pm.user_proc_help_init(u_proc, proc);
    start_user_proc(u_proc, user_start_addr);           // 使用用户进程启动的通用接口即可
}

bool ProcessManager::start_user_proc(proc_struct* proc, uint64 user_start_addr)
{
    // 此函数的接口才是符合用户进程定义的方式
    // 这里是用用户映像嵌入内核的方式实现进行测试的
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

    // 这个是为了进程调度上下文切换实现的
    // 进程的调度和上下文切换经过中断进行 进入S态后是内核完成的 
    // 因此完成切换用的上下文需要放在内核栈处 
    // 与内核进程不同 内核进程只在内核空间下进行 所以可以直接原地操作 sp指针就赋为内核栈地址即可
    // 用户进程有内核栈和用户栈 在进入中断时需要进行换指针的操作(可见trap入口的汇编)
    // 因此下面的sp指针需要设置为用户栈空间地址处 这是与内核进程启动不同的地方
    proc->context = (TRAPFRAME*)((char*)proc->kstack + proc->kstacksize) - 1;
    // 注意用户进程的权限设置
    uint64 SPIE = 1 << 5;                               // 100000
    uint64 SIE = 1 << 1;                                // 10
    uint64 SPP = 1 << 8;                                // 100000000
    proc->context->status = (read_csr(sstatus) | SPIE) & (~SPP) & (~SIE);
    proc->context->epc = user_start_addr;
    uint64 user_sp = EmbedUserProcessStackAddr + EmbedUserProcessStackSize;
    proc->context->reg.sp = user_sp;
    pm.switchstat_proc(proc, Proc_ready);
    return true;
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

// 从内核进程执行完特定的启动函数后进入用户进程
extern "C"
{
    void KerneltoUserProcess(proc_struct* proc, uint64 user_start_addr)
    {
        // 此时的进程还是在S态
        // 这个进程结构体已经被装载完毕
        // 或者说需要执行的启动函数已经执行完毕
        // 执行恢复用户进程的相关流程和跳转即可
        pm.switch_kernel_to_user(proc, user_start_addr);
        pm.switchstat_proc(proc, Proc_finished);
        asm volatile("li a7,2; ebreak");                // a7设置为2进行区分
        kout[yellow] << "This Block should not occur!" << endl;
    }
}

// 进程调度非常非常关键的调度器函数 本内核设计模式下所有的调度工作都将在这里实现
// 具有极强的灵活性和可扩展性
// 并且极好地实现了封装和隔离的高效的代码管理
TRAPFRAME* ProcessManager::proc_scheduler(TRAPFRAME* context)
{
    // 进程调度器的核心功能 利用trap进行进程调度
    // 调度器每次轮询时遇到需要回收的进程都会进行相应的回收

    // show_reg(context);

    if (cur_proc == nullptr)
    {
        kout[red] << "The Current Process is Nullptr!" << endl;
    }

    cur_proc->context = context;                    // 相当于进行的上下文的保存工作
    proc_struct* sptr = nullptr;
    if (need_rest)
    {
        sptr = cur_proc->next;                      // 从当前进程执行rest函数触发调度 那么应该跳过本进程的检查
        need_rest = false;
    }
    else
    {
        sptr = cur_proc;
    }

    // 检查是否有需要优先调度的进程 目前仅为单进程的优先调度算法
    // 后期有条件考虑实现为支持优先级或队列的优先级调度逻辑
    if (need_imme_run_proc != nullptr)
    {
        if (need_imme_run_proc->stat == Proc_ready)
        {
            if (cur_proc->stat == Proc_running)
            {
                pm.switchstat_proc(cur_proc, Proc_ready);
            }
            cur_proc = need_imme_run_proc;
            pm.switchstat_proc(cur_proc, Proc_running);
            cur_proc->vms->Enter();                 // 虚存管理地址空间的核心处理函数 要运行一个进程 就需要进入它的地址空间
            // 此进程已经被优先调度了 单逻辑公平机制下应该将优先调度指针置空
            need_imme_run_proc = nullptr;
            return cur_proc->context;
        }
    }

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
            cur_proc->vms->Enter();
            return cur_proc->context;
        }
        else
        {
            if (sptr->stat == Proc_finished || sptr->stat == Proc_zombie)
            {
                if (sptr->fa != nullptr)
                {
                    // 如果当前进程存在父进程的话
                    // 那么让父进程在它的逻辑里面去回收它
                    // 调度器不自动回收子进程
                    sptr = sptr->next;
                }
                else
                {
                    proc_struct* freed = sptr;
                    sptr = sptr->next;
                    pm.free_proc(freed);
                }
            }
            else
            {
                if (sptr->stat == Proc_sleeping)
                {
                    // 更新进程的sleeping时间
                    // 主要是为了进程自唤醒的设计
                    // 父子进程的wait可以通过父子进程的sem协调
                    // 但对于像sleep这样的系统调用需要能够自唤醒
                    // 对于自唤醒的设计需要考虑到使用两个信号量时
                    // 如果用于父子进程通信的信号量仍然阻塞
                    // 这里还是无法自唤醒的
                    pm.switchstat_proc(sptr, Proc_sleeping);
                    int w_value = sptr->sem->get_val();
                    if (w_value < 0)
                    {
                        // 说明当前进程由于父子进程间的关系
                        // 仍然阻塞在信号量上
                        // 还是无法继续进行自唤醒
                        kout[yellow] << "The process needs to wait its CHILD process!" << endl;
                        sptr = sptr->next;
                        continue;
                    }
                    if (sptr->sleeptime >= sptr->waittime_limit)
                    {
                        // 可以自唤醒了
                        // 并且可以进行调度了
                        pm.switchstat_proc(sptr, Proc_ready);
                        if (cur_proc->stat == Proc_running)
                        {
                            pm.switchstat_proc(cur_proc, Proc_ready);
                        }
                        cur_proc = sptr;
                        pm.switchstat_proc(cur_proc, Proc_running);
                        cur_proc->vms->Enter();
                        return cur_proc->context;
                    }
                }
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
            cur_proc->vms->Enter();
            return cur_proc->context;
        }
        else
        {
            if (sptr->stat == Proc_finished || sptr->stat == Proc_zombie)
            {
                if (sptr->fa != nullptr)
                {
                    // 如果父进程被回收了
                    // 我们的逻辑就是让它的所有子进程成为孤儿
                    // 然后让调度器自己回收 这样的设计更加统一简洁
                    sptr = sptr->next;
                }
                else
                {
                    proc_struct* freed = sptr;
                    sptr = sptr->next;
                    pm.free_proc(freed);
                }
            }
            else
            {
                if (sptr->stat == Proc_sleeping)
                {
                    // 更新进程的sleeping时间
                    // 主要是为了进程自唤醒的设计
                    // 父子进程的wait可以通过父子进程的sem协调
                    // 但对于像sleep这样的系统调用需要能够自唤醒
                    pm.switchstat_proc(sptr, Proc_sleeping);
                    int w_value = sptr->sem->get_val();
                    if (w_value < 0)
                    {
                        // 说明当前进程由于父子进程间的关系
                        // 仍然阻塞在信号量上
                        // 还是无法继续进行自唤醒
                        kout[yellow] << "The process needs to wait its CHILD process!" << endl;
                        sptr = sptr->next;
                        continue;
                    }
                    if (sptr->sleeptime >= sptr->waittime_limit)
                    {
                        // 可以自唤醒了
                        // 并且可以进行调度了
                        pm.switchstat_proc(sptr, Proc_ready);
                        if (cur_proc->stat == Proc_running)
                        {
                            pm.switchstat_proc(cur_proc, Proc_ready);
                        }
                        cur_proc = sptr;
                        pm.switchstat_proc(cur_proc, Proc_running);
                        cur_proc->vms->Enter();
                        return cur_proc->context;
                    }
                }
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

void ProcessManager::imme_trigger_schedule()
{
    // 本质逻辑是基于调度器的设计
    // 因此需要先打开中断
    // 不过需要判断上一次中断的状态 如果是关闭的那么还需要恢复
    bool need_back = false;
    if (!is_intr_enable())
    {
        need_back = true;
        interrupt_enable();
    }
    imme_schedule = 1;
    imme_trigger_timer();
    imme_schedule = 0;
    if (need_back)
    {
        interrupt_disable();
    }
}

void ProcessManager::run_proc(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    // 让一个进程立即run起来 其实就是让它成为调度器的下一个进程
    // 这个函数没有理由让一个进程独占时间片
    // 这个函数存在的目的仅是为了暂时提高这个进程的优先级并让它成为下一个需要调度的进程
    // 利用中断和本OS内核的进程调度逻辑可以发出一个立即调度的信号从而实现
    // 其实是通过这个函数实现了进程调度模块中的伪优先级调度算法
    // 不过设计逻辑上暂时只能支持一个进程的优先级调度
    // 后期条件允许考虑设计进程队列或者优先队列支持下的优先级调度算法...
    if (proc->stat == Proc_running)
    {
        // 如果进程已经在run 则没必要
        return;
    }
    if (proc->stat == Proc_ready)
    {
        // 我们的逻辑是只有就绪态的进程才能允许被立刻run
        // 而阻塞态的进程因为同步原语的设置只能被相关原语唤醒
        // 不应该提供强制run唤醒的接口 这会破坏同步原语的结构
        if (need_imme_run_proc == nullptr)
        {
            need_imme_run_proc = proc;
            imme_trigger_schedule();
            if (need_imme_run_proc == proc)
            {
                need_imme_run_proc = nullptr;
            }
        }
        else
        {
            kout[yellow] << "There has a Process need to run!" << endl;
        }
    }
}

void ProcessManager::rest_proc(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    // 让一个进程暂时暂停使用时间片 即阻塞
    // 不过这个只是让该进程立即停止执行手头的工作
    // 转而调度其他进程 下一次轮到它是取决于调度器RR轮询的逻辑
    // 和信号量中使用的wait队列的出队逻辑是不一致的
    if (proc->stat == Proc_running)
    {
        // 区别就在这里 仅是改变进程状态为就绪态 暂时停止执行 放出资源和时间片
        // 并非同步原语中的等待队列的逻辑 那个是进入了阻塞/睡眠态
        // 这个还是可以在任意时刻被唤醒的
        pm.switchstat_proc(proc, Proc_ready);
        need_rest = true;
        imme_trigger_schedule();
        if (need_rest)
        {
            need_rest = false;
        }
    }
}

void ProcessManager::kill_proc(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    // 强行杀死一个进程
    // 同样的逻辑 更改状态并调度 调度器中会自行回收
    pm.switchstat_proc(proc, Proc_zombie);
    kout[red] << "The Process whose pid is " << proc->pid << " has been Killed!" << endl;
    imme_trigger_schedule();
}

void ProcessManager::exit_proc(proc_struct* proc, int ec)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    // 进程退出函数
    // 主要是系统调用设计需要 ec为退出码
    if (ec != 0)
    {
        // 退出码非正常
        kout[yellow] << "The Process " << proc->pid << " exit with exit value " << ec << endl;
        pm.switchstat_proc(proc, Proc_zombie);
    }
    else
    {
        // 退出码正常
        kout[yellow] << "The process " << proc->pid << " exit successfully!" << endl;
        pm.switchstat_proc(proc, Proc_finished);
    }
    proc->exit_value = ec;
    proc->vms->Leave();
    // 如果当前进程有父进程
    // 那么出于wait的逻辑 当子进程被回收了应该可以让父进程signal
    if (proc->fa != nullptr)
    {
        proc->fa->sem->signal();
    }
    // 这里可以触发立即调度进行回收 也可以在时钟中断RR调度下轮询回收
    // 立即回收理论上只有当调用函数的进程为当前执行的进程时会有明显效果
    pm.imme_trigger_schedule();
}

bool ProcessManager::init_proc_fds(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // 进程创建时它的fd表的虚拟头节点应该已经创建好
    if (proc->fo_head == nullptr)
    {
        kout[red] << "The Process's fo head doesn't exist!" << endl;
        return false;
    }

    // 必须保证初始化时这三个fd是新生成的
    // 即进程的fd表中没有其他fd表项
    if (proc->fo_head->next != nullptr)
    {
        kout[yellow] << "The Process init FD TABLE has other fds!" << endl;
        // 那么就先释放掉所有的再新建头节点
        fom.free_all_flobj(proc->fo_head);
        fom.init_proc_fo_head(proc);
    }

    // 初始化一个进程时需要初始化它的文件描述符表
    // 即一个进程创建时会默认打开三个文件描述符 标准输入 输出 错误
    file_object* cur_fo_head = proc->fo_head;
    file_object* tmp_fo = nullptr;
    if (STDIO != nullptr)
    {
        // 只有当stdio文件对象非空时创建才有必有
        tmp_fo = fom.create_flobj(cur_fo_head, STDIN_FILENO);           // 标准输入
        fom.set_fo_file(tmp_fo, STDIO);
        fom.set_fo_flags(tmp_fo, O_RDONLY);
        tmp_fo = fom.create_flobj(cur_fo_head, STDOUT_FILENO);          // 标准输出
        fom.set_fo_file(tmp_fo, STDIO);
        fom.set_fo_flags(tmp_fo, O_WRONLY);
        tmp_fo = fom.create_flobj(cur_fo_head, STDERR_FILENO);          // 标准错误
        fom.set_fo_file(tmp_fo, STDIO);
        fom.set_fo_flags(tmp_fo, O_WRONLY);
    }
    return true;
}

bool ProcessManager::destroy_proc_fds(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // 释放一个进程的所有文件描述符表资源
    // 由于在fom封装层做了核心的工作
    // 这一层封装的关键妙处就在于对于进程来讲需要管控的资源只有fo文件对象结构体的资源
    // 而至于这个结构体里面的成员的相关资源在文件系统和fom类中已经分别做了处理
    // 使得各自完成各自的工作 层次清晰 互不干扰
    // 即进程只管理文件描述符表 没有权利直接管理文件 由OS进行整体的组织
    // 这里只需要调用相关函数即可
    if (proc->fo_head == nullptr)
    {
        // 已经为空
        return true;
    }
    fom.free_all_flobj(proc->fo_head);
    return true;
}

file_object* ProcessManager::get_spec_fd(proc_struct* proc, int fd_num)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return nullptr;
    }
    // 充分体现了在进程和文件增加一层fo的管理和封装的绝妙之处
    // 极大地方便了两边的管理和代码编写 逻辑和层次也非常清晰
    file_object* ret_fo = nullptr;
    ret_fo = fom.get_from_fd(proc->fo_head, fd_num);
    if (ret_fo == nullptr)
    {
        kout[red] << "The Process can not get the specified FD!" << endl;
        return ret_fo;
    }
    return ret_fo;
}

bool ProcessManager::copy_proc_fds(proc_struct* dst, proc_struct* src)
{
    if (dst == nullptr || src == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return false;
    }

    // 对于进程间的文件描述符表的拷贝
    // 使用最简单朴素的方式 即删除原来的 复制新的
    // 不过需要注意复制新的并不能仅仅做指针的迁移
    // 因为如上所述 fo即fom的封装与设计极好地分割又统一联系了进程和文件系统间的关系
    // 而且fo是进程的下层接口 文件系统是进程的上层接口
    // 因此进程只要按照它需要的逻辑任意操作fo即可 不需要考虑文件系统的组织问题
    fom.free_all_flobj(dst->fo_head);
    fom.init_proc_fo_head(dst);
    file_object* dst_fo_head = dst->fo_head;
    file_object* dst_fo_ptr = dst_fo_head;
    file_object* src_fo_ptr = nullptr;
    for (src_fo_ptr = src->fo_head->next;src_fo_ptr != nullptr;src_fo_ptr = src_fo_ptr->next)
    {
        file_object* new_fo = fom.duplicate_fo(src_fo_ptr);
        if (new_fo == nullptr)
        {
            kout[red] << "File Object Duplicate Error!" << endl;
        }
        dst_fo_ptr->next = new_fo;
        dst_fo_ptr = dst_fo_ptr->next;
    }
    return true;
}

proc_struct* CreateKernelProcess(int (*func)(void*), void* arg, char* name)
{
    proc_struct* k_proc = pm.alloc_proc();
    pm.init_proc(k_proc, 1);
    pm.set_proc_name(k_proc, name);
    pm.set_proc_kstk(k_proc, nullptr, KERNELSTACKSIZE * 4);
    pm.set_proc_vms(k_proc, VMS::GetKernelVMS());
    pm.start_kernel_proc(k_proc, func, arg);
    return k_proc;
}

proc_struct* CreateUserImgProcess(uint64 img_start, uint64 img_end, char* name)
{
    // 创建用户进程
    // 需要利用到虚存的相关机制
    VMS* vms = (VMS*)kmalloc(sizeof(VMS));
    vms->init();
    uint64 img_load_size = img_end - img_start;
    uint64 img_load_start = EmbedUserProcessEntryAddr;
    uint64 img_load_end = img_load_start + img_load_size;
    int user_bin_flag = VM_rwx;                                             // 用户映像的权限 可读可写可执行
    vms->insert(img_load_start, img_load_end, user_bin_flag);               // 创建用户映像空间在虚存中
    uint64 img_user_stack_size = EmbedUserProcessStackSize;
    uint64 img_user_stack_start = EmbedUserProcessStackAddr;
    uint64 img_user_stack_end = img_user_stack_start + img_user_stack_size;
    int user_stack_flag = VM_userstack;                                     // 用户栈权限 可读可写栈动态
    vms->insert(img_user_stack_start, img_user_stack_end, user_stack_flag); // 分配用户栈空间

    // 创建完用户的vms后就是创建进程的流程了
    proc_struct* u_proc = pm.alloc_proc();
    pm.init_proc(u_proc, 2);
    pm.set_proc_name(u_proc, name);
    pm.set_proc_kstk(u_proc, nullptr, KERNELSTACKSIZE * 4);
    pm.set_proc_vms(u_proc, vms);
    pm.start_user_proc(u_proc, EmbedUserProcessEntryAddr);

    // test 能真正运行出想要的效果
    u_proc->vms->Enter();                                                   // 进入vms
    u_proc->vms->EnableAccessUser();                                        // 设置允许内核态访问用户空间 riscv的具有的一个点
    memcpy((void*)img_load_start, (char*)img_start, img_load_size);         // 将嵌入内核的用户映像地址拷贝到用户空间执行地址处
    u_proc->vms->DisableAccessUser();                                       // 上面执行完了 这里就反过来
    u_proc->vms->Leave();                                                   // 离开vms

    return u_proc;
}

proc_struct* CreateUserImgfromFunc(int (*func)(void*), void* arg, uint64 img_start, uint64 img_end, char* name)
{
    VMS* vms = (VMS*)kmalloc(sizeof(VMS));
    vms->init();
    uint64 img_load_size = img_end - img_start;
    uint64 img_load_start = EmbedUserProcessEntryAddr;
    uint64 img_load_end = img_load_start + img_load_size;
    int user_bin_flag = VM_rwx;
    vms->insert(img_load_start, img_load_end, user_bin_flag);
    uint64 img_user_stack_size = EmbedUserProcessStackSize;
    uint64 img_user_stack_start = EmbedUserProcessStackAddr;
    uint64 img_user_stack_end = img_user_stack_start + img_user_stack_size;
    int user_stack_flag = VM_userstack;
    vms->insert(img_user_stack_start, img_user_stack_end, user_stack_flag);

    proc_struct* u_proc = pm.alloc_proc();
    pm.init_proc(u_proc, 2);
    pm.set_proc_name(u_proc, name);
    pm.set_proc_kstk(u_proc, nullptr, KERNELSTACKSIZE * 4);
    pm.set_proc_vms(u_proc, vms);
    pm.start_user_proc(u_proc, func, arg, EmbedUserProcessEntryAddr);

    u_proc->vms->Enter();
    u_proc->vms->EnableAccessUser();
    memcpy((void*)img_load_start, (char*)img_start, img_load_size);
    u_proc->vms->DisableAccessUser();
    u_proc->vms->Leave();

    return u_proc;
}

ProcessManager pm;