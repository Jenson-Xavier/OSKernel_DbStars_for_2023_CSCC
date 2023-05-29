#include <systemcall.hpp>
#include <userlibs.hpp>

int stack[1024] = { 0,0,1,1,1 };

struct tms
{
    long tms_utime;
    long tms_stime;
    long tms_cutime;
    long tms_cstime;
};

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

struct tms mytimes = { 1,1,1,1 };

struct utsname un = { "1" };

static int child_func(void) {
    u_puts("Child says successfully!\n");
    int ppid = getppid();
    // u_puts((int64)ppid);
    int test_ret = times(&mytimes);
    u_puts("CHILD TIME::\n");
    u_puts((int64)test_ret);
    u_puts((int64)mytimes.tms_utime);
    u_puts((int64)mytimes.tms_stime);
    u_puts((int64)mytimes.tms_cutime);
    u_puts((int64)mytimes.tms_cstime);
    return 0;
}

void test_clone(void) {
    int wstatus;
    constexpr int SIGCHLD = 17;
    int child_pid = 0;
    child_pid = clone(child_func, nullptr, stack, 1024, SIGCHLD);
    if (child_pid == 0)
    {
        // 子线程的执行块
        // 但是clone的逻辑使子线程仅仅去执行一个函数了
        // 不会执行到这里
        u_exit(0);
    }
    else
    {
        u_puts("Father Thread is Waiting......\n");
        int pid = getpid();
        u_puts((int64)pid);
        if (wait(&wstatus) == child_pid)
        {
            u_puts("This is the Father Thread!\n");
            u_puts("The Child Thread's pid is ");
            u_puts((int64)child_pid);
            u_puts("Child Thread exit wstatus is ");
            u_puts((int64)wstatus);
            u_puts("FATHER TIME::\n");
            int test_ret = times(&mytimes);
            u_puts((int64)test_ret);
            u_puts((int64)mytimes.tms_utime);
            u_puts((int64)mytimes.tms_stime);
            u_puts((int64)mytimes.tms_cutime);
            u_puts((int64)mytimes.tms_cstime);
        }
        else
        {
            u_puts("Wait Error!\n");
        }
    }
}

void test_getpid()
{
    int pid = getpid();
    u_puts("getpid success.\n");
    u_puts((int64)pid);
}

void test_getppid()
{
    int ppid = getppid();
    if (ppid > 0)
    {
        u_puts("getppid success.\n");
    }
    else
    {
        u_puts("getppid error.\n");
    }
    u_puts((int64)ppid);
}

void test_times()
{
    int test_ret = times(&mytimes);
    u_puts((int64)test_ret);
    u_puts((int64)mytimes.tms_utime);
    u_puts((int64)mytimes.tms_stime);
    u_puts((int64)mytimes.tms_cutime);
    u_puts((int64)mytimes.tms_cstime);
}

void test_uname()
{
    int ret = uname(&un);
    u_puts((int64)ret);
    u_puts(un.sysname);
    u_puts(un.nodename);
    u_puts(un.release);
    u_puts(un.version);
    u_puts(un.machine);
    u_puts(un.domainname);
}

void test_sleep()
{
    int time1 = get_time();
    u_puts(time1);
    int ret = sleep(1);
    u_puts(ret);
    int time2 = get_time();
    u_puts(time2);
    if (time2 - time1 >= 1)
    {
        u_puts("sleep success.");
    }
    else
    {
        u_puts("sleep error.");
    }
}

int main(void) {
    // test_clone();
    // test_getpid();
    // test_getppid();
    // test_times();
    // test_uname();
    test_sleep();
    return 0;
}