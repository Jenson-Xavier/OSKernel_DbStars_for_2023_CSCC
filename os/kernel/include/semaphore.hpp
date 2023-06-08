#ifndef __SEMAPHORE_HPP__
#define __SEMAPHORE_HPP__

// 通过自旋锁完成内核的信号量设计
// 信号量是本内核实现同步原语的核心设计 其他同步原语暂不考虑(有条件和需求时会考虑)

#include <process.hpp>

// 为了实现信号量 自定义进程队列
// 为了设计的合理安排和部署 这里的队列虽然采取链表结构
// 但是不直接在进程结构体上进行 原队列的next指针是和分配内存空间上的进程链表
// 使用一个结构体进行封装
// 类封装方法 统一使用公有成员即可
struct ListNode
{
    proc_struct* proc;
    ListNode* next;
};

// 视作进程队列的类型
struct proc_queue
{
    ListNode* front;
    ListNode* rear;
};

class ProcessQueueManager
{
public:
    void print_all_queue(proc_queue& pq);
    void init(proc_queue& pq);
    void destroy_pq(proc_queue& pq);
    bool isempty_pq(proc_queue& pq);
    int length_pq(proc_queue& pq);
    void enqueue_pq(proc_queue& pq, proc_struct* ins_proc);
    proc_struct* front_pq(proc_queue& pq);
    void dequeue_pq(proc_queue& pq);
};

extern ProcessQueueManager pqm;

// 信号量semaphore实现
// 采取标准的semaphore实现方式
// 阻塞唤醒方式为队列FIFO 设计的信号量为强信号量
// 在执行方面的逻辑主要是通过进程的状态和调度器的逻辑去实现
class SEMAPHORE
{
protected:
    int value;                                              // 信号量的值
    // 概念上讲 
    // value值为正时 可以表示为发出semWait后可以立即继续执行的进程的数量
    // value值为零时 可以表示为发出semWait后的下一个操作会被阻塞
    // value值为负时 其绝对值是等待解除阻塞的进程的数量 semSignal会使等待队列的下一个进程解除阻塞
    proc_queue wait_pq;                                     // 信号量的等待队列 存放的是进程单元

public:
    inline int get_val()
    {
        return value;
    }

    inline proc_queue get_wtpq()
    {
        return wait_pq;
    }

    void init(int init_val = 0);                            // 初始化信号量初值 可以指定值 默认为0
    // 目前仅考虑单核CPU下的OS内核 故默认参数可以认为是当前执行的进程
    int wait(proc_struct* proc = pm.get_cur_proc());        // 信号量的经典操作 semWait
    void signal(proc_struct* proc = pm.get_cur_proc());     // 信号量的经典操作 semSignal
    bool destroy();                                         // 为单独的进程设计需要 释放信号量上相应的空间
};

extern SEMAPHORE semaphore;

#endif