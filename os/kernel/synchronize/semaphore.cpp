#include <semaphore.hpp>
#include <pmm.hpp>
#include <kout.hpp>
#include <spinlock.hpp>
#include <mutex.hpp>
#include <interrupt.hpp>
#include <clock.hpp>

// 实现进程队列的部分

void ProcessQueueManager::print_all_queue(proc_queue& pq)
{
    if (pq.front == nullptr || pq.front->next == nullptr)
    {
        return;
    }
    ListNode* ptr = pq.front->next;
    while (ptr != nullptr)
    {
        pm.show(ptr->proc);
        ptr = ptr->next;
    }
}

void ProcessQueueManager::init(proc_queue& pq)
{
    pq.front = (ListNode*)kmalloc(sizeof(struct ListNode));
    pq.rear = pq.front;
    if (pq.front == nullptr)
    {
        kout[red] << "Process Queue Init Falied!" << endl;
        return;
    }
    pq.front->next = nullptr;
}

void ProcessQueueManager::destroy_pq(proc_queue& pq)
{
    ListNode* ptr = pq.front;
    while (ptr != nullptr)
    {
        ListNode* t = ptr->next;
        ptr = ptr->next;
        kfree(t);
    }
    pq.front = pq.rear = nullptr;
}

bool ProcessQueueManager::isempty_pq(proc_queue& pq)
{
    ListNode* ptr = pq.front;
    if (ptr == nullptr)
    {
        return true;
    }
    if (ptr->next != nullptr)
    {
        return true;
    }
    return false;
}

int ProcessQueueManager::length_pq(proc_queue& pq)
{
    ListNode* ptr = pq.front;
    if (ptr == nullptr || ptr->next == nullptr)
    {
        return 0;
    }
    ptr = ptr->next;
    int ret_len = 0;
    while (ptr != nullptr)
    {
        ret_len++;
        ptr = ptr->next;
    }
    return ret_len;
}

void ProcessQueueManager::enqueue_pq(proc_queue& pq,proc_struct* ins_proc)
{
    ListNode* newnode = (ListNode*)kmalloc(sizeof(struct ListNode));
    newnode->proc = ins_proc;
    pq.rear->next = newnode;
    pq.rear = pq.rear->next;
}

proc_struct* ProcessQueueManager::front_pq(proc_queue& pq)
{
    if (pq.front == pq.rear)
    {
        kout[yellow] << "The Process Queue is Empty!" << endl;
        return nullptr;
    }
    proc_struct* ret = pq.front->next->proc;
    return ret;
}

void ProcessQueueManager::dequeue_pq(proc_queue& pq)
{
    if (pq.front == pq.rear)
    {
        kout[yellow] << "The Process Queue is Empty!" << endl;
        return;
    }
    ListNode* t = pq.front->next;
    pq.front->next = t->next;
    if (pq.rear == t)pq.rear = pq.front;
    kfree(t);
}

ProcessQueueManager pqm;

/*------------------------------------------------*/

// 实现信号量的部分
void SEMAPHORE::init(int intr_val)
{
    if (intr_val < 0)
    {
        // 信号量的值不应该为负的
        kout[red] << "Semaphore's Value Init Should not be negative!" << endl;
        return;
    }
    value = intr_val;
    pqm.init(wait_pq);
}

int SEMAPHORE::wait(proc_struct* proc)
{
    // 关闭终端和加自旋锁都是为了保证信号量操作的原子性
    bool intr_flag;
    intr_save(intr_flag);
    spin_lock.lock();
    if (value <= 0)
    {
        // 当前发出wait信号的进程应该阻塞
        pqm.enqueue_pq(wait_pq, proc);
        // 在进程执行流上的改变大抵就是状态的改变
        // 调度器会自行区分
        // 事实上这里wait_queue的作用就是确定接触阻塞的顺序(公平的FIFO)
        // 这样强信号量的设计可以被证明可以保证互斥场景下不会饥饿的情况
        // 而如果不设计队列而只是改变状态这里就会可能出现饥饿的情况
        pm.switchstat_proc(proc, Proc_sleeping);
        // 立即触发调度器 从而实现立即的进程切换功能
        value--;
    }
    else
    {
        value--;
        // 接下来仍然是正常的时间片轮转调度的执行
    }
    spin_lock.unlock();
    intr_restore(intr_flag);
    return value;
}

void SEMAPHORE::signal(proc_struct* proc)
{
    bool intr_flag;
    intr_save(intr_flag);
    spin_lock.lock();
    if (value < 0)
    {
        // 会唤醒一个进程 即将一个进程从等待队列中移除
        // 这并不是立即执行的意思 所以这里不需要立即触发调度器
        proc_struct* proc = pqm.front_pq(wait_pq);
        pqm.dequeue_pq(wait_pq);
        pm.switchstat_proc(proc, Proc_ready);
        value++;
    }
    else
    {
        value++;
    }
    spin_lock.unlock();
    intr_restore(intr_flag);
}

SEMAPHORE semaphore;