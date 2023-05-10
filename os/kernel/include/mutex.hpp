#ifndef __MUTEX_HPP__
#define __MUTEX_HPP__

#include <spinlock.hpp>
#include <process.hpp>

// 互斥锁根据阻塞策略也可以直接使用自旋锁来实现 
// 关键是要区分解锁和加锁必须是同一个进程
// 之后如果有条件可以考虑改进阻塞策略实现更先进的互斥锁
// 其实也可以在调用自旋锁的时候注意调用方式从而区分加锁和解锁的进程
// 相比较使用权限更加宽泛的自旋锁 自旋锁允许任意进程加锁解锁
// 因此无法避免当前进程加锁的临界区被另外一个进程恶意解锁加以修改
// 互斥锁就是增加了这一层更加先进完善的保护机制
// 某个进程获取了互斥锁 只有它自己可以解锁 其他进程无论无何无法擅自解锁进入临界区
// 但是阻塞机制仍采取的自旋的策略 有待改进

class MUTEX_LOCK
{
protected:
    // belong_pid为0表示互斥锁还没有分配 否则就表示被某个所属进程号获取
    int belong_pid = 0;
    volatile bool spin_lock_value = 0;          // 阻塞策略暂使用自旋锁的机制
    
public:
    inline int get_blid()
    {
        return belong_pid;
    }

    inline bool get_slv()
    {
        return spin_lock_value;
    }

    inline void init()
    {
        belong_pid = 0;
        spin_lock_value = 0;
    }

    bool try_mutex(int call_pid = pm.get_cur_proc()->pid);      // 尝试能否加锁 默认参数是当前的进程id
    void enter_mutex(int call_pid = pm.get_cur_proc()->pid);    // 传入需要获得锁的进程号 加锁
    void release_mutex(int call_pid = pm.get_cur_proc()->pid);  // 解锁
};

extern MUTEX_LOCK mutex;

#endif