#include <systemcall.hpp>
#include <klib.hpp>

// 进入用户进程

int main();

extern "C" void _usermain()
{
    // 所有用户进程执行完main函数return 0之后都会通过exit系统调用退出进程
    int ret = main();
    u_exit(ret);
}