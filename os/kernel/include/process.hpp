#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

// 初步实现进程管理 先构建起内核线程 进而构建其用户线程

#include <type.hpp>
#include <memory.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <resources.hpp>

// 内核栈设置
#define KERNELSTACKSIZE      4096

// 用户栈设置
#define EmbedUserProcessEntryAddr   0x800020
#define EmbedUserProcessStackSize   4096 * 32
#define EmbedUserProcessStackAddr   0x80000000 - EmbedUserProcessStackSize

class VMS;                  // 前置声明 可以作为类型引用

// 定义控制进程的进程单元结构体 即实现进程控制块PCB 
// 当然还有需要进程切换的进程上下文Context

// 使用枚举类型 定义我们OS的进程控制所含有的状态
// 采取六个状态来实现
enum proc_status
{
    Proc_allocated = 0,     // 分配态 表示一个刚刚分配好内存的进程 还没有进行相关初始化(uninit) 
    Proc_ready,             // 就绪态 表示当前进程完成了初始化任务
    Proc_running,           // 执行态 表示当前进程正在执行
    Proc_sleeping,          // 挂起/等待态 表示进程因为中断或异常等由执行态挂起进入等待重新分配资源
    Proc_finished,          // 终止态 表示进程完成了所需完成的任务等待回收相关资源
    Proc_zombie,            // 僵死态 表示进程已经终止但是进程表还有相关的结构和资源
};

// 后续为了更好地管理和隔离 使设计结构清晰化 摒弃这样的轻量级上下文的设计
// 而是利用中断trap进行进程上下文的恢复和处理 直接使用trapframe结构体即可
// 进程上下文 参考ucore设计 利用函数调用规则仅用14个寄存器实现进程上下文的保存
struct reg_context
{
    uint64 ra;
    uint64 sp;
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

extern "C"
{
    extern char boot_stack[];
    extern char boot_stack_top[];
    extern char KernelProcessEntry[];
    extern char UserProcessEntry[];
};

#define PROC_NAME_LEN   50      // 进程名字的最大长度 待完成完善字符串处理函数后继续实现
#define MAX_PROCESS     1024    // 最大进程数量 其实我们的内核应用应该不会用到这么多
#define MAX_PID         (MAX_PROCESS * 2)   // *2去分配进程号 hash思想 更好避免冲突

// 进程控制块结构体 进程管理的核心结构体
struct proc_struct
{
    proc_struct* next;          // 统一方便进程链表的维护 因为总进程数量不会太大 使用单链表结构
    uint32 stat;                // 进程状态
    int pid;                    // 进程号标识符

    clock_t timebase;           // Round Robin时间片轮转调度实现需要 计时起点
    clock_t runtime;            // 进程运行的时间
    clock_t sleeptime;          // 挂起态等待时间(保留设计 暂不使用)
    clock_t readytime;          // 就绪态等待时间(保留设计 暂不使用)

    int ku_flag;                // flag标志是内核级还是用户级进程
    
    void* kstack;               // 内核栈地址
    uint32 kstacksize;          // 内核栈大小 rv64下默认采取16KB实现
    
    // 链接部分
    proc_struct* pid_fa;        // 到父进程的进程结构体 这里使用指针 方便确保进程树的关系不乱
    proc_struct* pid_bro_pre;   // 到兄弟进程 pre方向
    proc_struct* pid_bro_next;  // 到兄弟进程 next方向
    proc_struct* pid_fir_child; // 到第一个孩子进程
    
    // reg_context context;     // 轻量级上下文 弃用...
    TRAPFRAME* context;         // 利用trapframe结构进行进程上下文的处理 更加清晰和统一 其直接分配在内核栈上

    VMS* vms;                   // 虚拟内存关键VMS指针 管理虚存和物存空间
    
    char name[PROC_NAME_LEN];   // 进程名称

    int exit_value;             // 后续系统调用需要 退出状态值(具体使用场景暂无)
    
    // ... IPC进程通信部分
    
    // ... File System文件系统部分
};

// 声明全局 OS第0个进程idle_proc
extern struct proc_struct* idle_proc;

// 声明一个立即调度的bool变量 用来触发立即调度实现同步原语或进程调度 即阻塞或者立即执行的功能
extern bool imme_schedule;

// 进程管理和控制的类 也可认为是进程调度器
class ProcessManager
{
protected:
    proc_struct proc_listhead;                      // 进程链表头节点
    proc_struct* cur_proc;                          // 当前正在执行的进程
    proc_struct* need_imme_run_proc;                // 需要立即调度的进程 为了设计run_proc函数
    uint32 proc_count;                              // 存在的进程数量
    void add_proc_tolist(proc_struct* proc);        // 将分配好的进程加入进程链表
    bool remove_proc_fromlist(proc_struct* proc);   // 将已经加入进程链表的进程移除
    void init_idleproc_for_boot();                  // 创建并初始化第0个idle进程 相当于boot进程
    int finish_proc(proc_struct* proc);             // 结束一个进程 释放其所属的资源 这里最好有对应的退出码设置
    bool need_rest;                                 // 标记是否需要休息当前的进程 调度使用

public:
    // PM类自身相关的操作
    void Init();                                    // PM类初始化函数
    void show(proc_struct* proc);                   // 打印一个proc结构相关信息 用于调试
    void print_all_list();                          // 打印进程链表所有进程节点信息 便于调试
    
    proc_struct* alloc_proc();                      // 管理分配一个新进程
    int get_unique_pid();                           // 得到一个独特的pid用来分配
    bool init_proc(proc_struct* proc, int ku_flag); // 初始化一个进程
    proc_struct* get_proc(int pid);                 // 从进程号pid得到一个进程实例
    bool free_proc(proc_struct* proc);              // 释放一个以及分配的进程的资源和空间
    
    inline proc_struct* get_cur_proc()              // 获取当前正在执行的进程实例
    {
        return cur_proc;
    }

    inline uint32 get_proc_count()                  // 获取存在的进程数量
    {
        return proc_count;
    }

    inline bool is_needrest()                       // 获取是否需要rest的信息
    {
        return need_rest;
    }

    // 进程调度核心 基于这样的进程调度设计非常具有可扩展性和灵活性
    TRAPFRAME* proc_scheduler(TRAPFRAME* context);   // 进行进程调度的关键函数 主要是时间片轮转 trap实现 故这里的返回值如此设置
    void imme_trigger_schedule();                    // 实现一个立即触发调度器 辅助且统一实现本OS设计下的进程调度模块

    // PM类管理proc_struct相关的操作
    bool switchstat_proc(proc_struct* proc, uint32 tar_stat);               // 改变一个进程的状态
    bool set_proc_name(proc_struct* proc, char* name);                      // 设置一个进程的名称
    bool set_proc_kstk(proc_struct* proc, void* kaddr, uint32 size);        // 设置进程内核栈
    bool copy_otherprocs(proc_struct* dst, proc_struct* src);               // 进程间的拷贝
    void run_proc(proc_struct* proc);                                       // 暂优先运行一个进程
    void rest_proc(proc_struct* proc);                                      // 暂停一个进程
    void kill_proc(proc_struct* proc);                                      // 杀死一个进程
    void exit_proc(proc_struct* proc, int ec);                              // 进程退出函数
    bool set_proc_vms(proc_struct* proc, VMS* vms);                         // 设置进程的VMS属性
    
    bool start_kernel_proc(proc_struct* proc,       // 通过给定的函数启动内核线程
        int (*func)(void*), void* arg);             
    bool start_user_proc(proc_struct* proc,         // 通过给定的函数启动用户进程(内核创建用户函数 几乎不会使用)
        int (*func)(void*), void* arg);
    bool start_user_proc(proc_struct* proc,         // 通过用户映像启动用户进程
        uint64 user_start_addr);
        // ... 文件系统相关
};

// 创建内核进程的通用全局函数
proc_struct* CreateKernelProcess(int (*func)(void*), void* arg, char* name);

// 创建用户进程的全局函数(内核嵌入用户进程映像实现的)
proc_struct* CreateUserImgProcess(uint64 img_start, uint64 img_end, char* name = (char*)"");

extern ProcessManager pm;

#endif