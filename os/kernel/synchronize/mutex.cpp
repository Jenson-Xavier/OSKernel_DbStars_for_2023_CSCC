#include <mutex.hpp>
#include <kout.hpp>

// 除了进程号的判断整体逻辑基本复用的自旋锁部分

bool MUTEX_LOCK::try_mutex(int call_pid)
{
    if (belong_pid == 0)
    {
        // 当前锁还没有所属进程
        // 直接使用gcc编译器提供的机器指令原语实现即可
        bool old_lock = __sync_lock_test_and_set(&spin_lock_value, 1);
        if (!old_lock)
        {
            // 原语更新mutex的所属进程号
            bool tof = __sync_bool_compare_and_swap(&belong_pid, belong_pid, call_pid);
            __sync_synchronize();
            return !old_lock;
        }
        __sync_synchronize();
        return !old_lock;
    }
    else
    {
        // mutex锁有了所属进程
        if (call_pid == belong_pid)
        {
            kout[yellow] << "It is the same process to try enter mutex!" << endl;
            return true;
        }
        else
        {
            kout[red] << "Not the belong process so cannot try enter mutex!" << endl;
            return false;
        }
    }
}

void MUTEX_LOCK::enter_mutex(int call_pid)
{
    if (call_pid == belong_pid)return;
    while (__sync_lock_test_and_set(&spin_lock_value, 1) != 0);
    __sync_bool_compare_and_swap(&belong_pid, belong_pid, call_pid);
    __sync_synchronize();
}

void MUTEX_LOCK::release_mutex(int call_pid)
{
    if (belong_pid == 0 || belong_pid == call_pid)
    {
        __sync_synchronize();
        __sync_lock_release(&spin_lock_value);
        __sync_bool_compare_and_swap(&belong_pid, belong_pid, 0);
    }
    else
    {
        kout[red] << "Not the belong process so cannot release mutex!" << endl;
    }
}

MUTEX_LOCK mutex;