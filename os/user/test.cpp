#include <systemcall.hpp>

// 最简单的测试系统调用的用户程序

int main()
{
    // char ch = u_getchar();
    char ch = 'U';
    u_putchar(ch);
    return 0;
}