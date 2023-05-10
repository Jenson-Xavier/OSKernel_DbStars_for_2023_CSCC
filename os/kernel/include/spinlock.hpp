#ifndef __SPINLOCK_HPP__
#define __SPINLOCK_HPP__

// 实现OS kernel的同步原语部分
// 自旋锁的设计与实现
// spinlock锁机制是一个比较简单的忙等待方式的锁机制
// 本质上是通过内存中的一个整数来实现锁
// 任何进程进入临界区都需要检查这个值是否为0 为0则可以进入临界区 并设置其值为1 加锁
// 为1则忙等待解锁
// 具体实现可以利用gcc编译器提供的原语
// 使用类实现

class SPIN_LOCK
{
protected:
    volatile bool spin_lock_value = 0;
public:
    inline bool get_slv()
    {
        return spin_lock_value;
    }

    inline void init()
    {
        spin_lock_value = 0;
    }

    bool try_lock();                    // 一个进程尝试加锁的函数
    void lock();                        // 自旋锁自旋的过程 忙等待直至可以使用锁
    void unlock();                      // 解锁
};

extern SPIN_LOCK spin_lock;             // 声明类对象引用头文件可全局使用

#endif