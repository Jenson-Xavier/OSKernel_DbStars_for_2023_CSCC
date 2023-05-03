#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

// 初步实现进程管理 先构建起内核线程 进而构建其用户线程

#include <type.hpp>
#include <pmm.hpp>
#include <trap.hpp>

#define KERNELSTACKSIZE      4096

// 定义控制进程的进程单元结构体 即实现进程控制块PCB 
// 当然还有需要进程切换的进程上下文Context

// 使用枚举类型 定义我们OS的进程控制所含有的状态
// 采取六个状态来实现
enum proc_status
{
    Proc_allocated = 0,     // 分配态 表示一个刚刚分配好内存的进程 还没有进行相关初始化 
    Proc_ready,             // 就绪态 表示当前进程完成了初始化任务
    Proc_running,           // 执行态 表示当前进程正在执行
    Proc_sleeping,          // 挂起/等待态 表示进程因为中断或异常等由执行态挂起进入等待重新分配资源
    Proc_finished,          // 终止态 表示进程完成了所需完成的任务等待回收相关资源
    Proc_zombie,            // 僵死态 表示进程已经终止但是进程表还有相关的结构和资源
};

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
};

#define PROC_NAME_LEN   50      // 进程名字的最大长度 待完成完善字符串处理函数后继续实现
#define MAX_PROCESS     1024    // 最大进程数量 其实我们的内核应用应该不会用到这么多
#define MAX_PID         (MAX_PROCESS * 2)   // *2去分配进程号 hash思想 更好避免冲突

// 进程控制块结构体 进程管理的核心结构体
struct proc_struct
{
    proc_struct* next;          // 统一方便进程链表的维护 因为总进程数量不会太大 使用单链表结构
    enum proc_status stat;      // 进程状态
    int pid;                    // 进程号标识符
    
    // ... 调度信息 进程切换和进程调度需要
    // ... 其实这里是和时间有关的部分 在后面进程调度时再补充
    //int64 runtime;            //  进程运行的时间
    
    int ku_flag;                // flag标志是内核级还是用户级进程
    
    void* kstack;               // 内核栈地址
    uint32 kstacksize;          // 内核栈大小 rv64下默认采取16KB实现
    
    // 链接部分
    int pid_fa;                 // 到父进程的pid 这里直接用pid作为映射与指针相区分 用一点效率换取简便的设计
    int pid_bro_pre;            // 到兄弟进程 pre方向
    int pid_bro_next;           // 到兄弟进程 next方向
    int pid_fir_child;          // 到第一个孩子进程
    
    reg_context context;        // 轻量级上下文
    
    // ... 虚拟内存管理部分 以及地址空间 扩展用户进程
    
    char name[PROC_NAME_LEN];   // 进程名称 后续有需求再完善字符串处理函数
    
    // ... IPC进程通信部分
    
    // ... File System文件系统部分
};

// 声明全局 OS第0个进程idle_proc
extern struct proc_struct* idle_proc;


// 进程管理和控制的类 也可认为是进程调度器
class ProcessManager
{
protected:
    static proc_struct proc_listhead;                       // 进程链表头节点
    static proc_struct* cur_proc;                           // 当前正在执行的进程
    static uint32 proc_count;                               // 存在的进程数量
    static void add_proc_tolist(proc_struct* proc);         // 将分配好的进程加入进程链表
    static bool remove_proc_fromlist(proc_struct* proc);    // 将已经加入进程链表的进程移除

public:
    // PM类自身相关的操作
    void Init();                                    // PM类初始化函数

    proc_struct* alloc_proc();                      // 管理分配一个新进程
    int get_unique_pid();                           // 得到一个独特的pid用来分配
    bool init_proc(proc_struct* proc, int ku_flag); // 初始化一个进程
    proc_struct* get_proc(int pid);                 // 从进程号pid得到一个进程实例
    bool free_proc(proc_struct* proc);              // 释放一个以及分配的进程的资源和空间
    void init_idleproc_for_boot();                  // 创建并初始化第0个idle进程 相当于boot进程
    
    inline proc_struct* get_cur_proc()              // 获取当前正在执行的进程实例
    {
        return cur_proc;
    }

    inline uint32 get_proc_count()                  // 获取存在的进程数量
    {
        return proc_count;
    }

    void proc_schedule();                           // 进行进程调度的关键函数 主要是时间片轮转

    // PM类管理proc_struct相关的操作

    bool switchstat_proc(proc_struct* proc, uint32 tar_stat);               // 改变一个进程的状态
    bool set_proc_name(proc_struct* proc, char* name);                      // 设置一个进程的名称
    bool set_proc_kstk(proc_struct* proc, void* kaddr, uint32 size);        // 设置进程内核栈
    bool copy_otherprocs(proc_struct* dst, proc_struct* src);               // 进程间的拷贝
    bool run_proc(proc_struct* proc);               // 执行一个进程
    int finish_proc(proc_struct* proc);             // 结束一个进程 释放其所属的资源 这里最好有对应的退出码设置
    bool start_kernel_proc(proc_struct* proc,
        int (*func)(void*), void* arg);             // 通过给定的函数启动内核线程 用户级后续
    
    // ... 进程调度和切换需相关函数

    // ... 文件系统相关

    // ... 虚拟内存管理 堆栈区域管理相关 涉及用户进程
};

extern ProcessManager pm;

#endif