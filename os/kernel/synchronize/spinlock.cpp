#include <spinlock.hpp>

// 利用gcc编译器提供的原语简单实现自旋锁
// 本质上的原理就是利用硬件或者机器语言的支持实现

bool SPIN_LOCK::try_lock()
{
    // 设置新值 返回旧值 并加锁
    bool old_lock = __sync_lock_test_and_set(&spin_lock_value, 1);
    __sync_synchronize();               // 可以理解成提供全局内存屏障(memory barrier)
    return !old_lock;                   // 只有当old_lock为0时表示可以进入临界区并加锁
}

void SPIN_LOCK::lock()
{
    // 自旋锁的忙等待请求锁
    while (__sync_lock_test_and_set(&spin_lock_value, 1) != 0)
    {
        // 忙等待
    };
    __sync_synchronize();
}

void SPIN_LOCK::unlock()
{
    __sync_synchronize();
    __sync_lock_release(&spin_lock_value);
}

SPIN_LOCK spin_lock;