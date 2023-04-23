#include <sbi.h>
#include <kout.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <Riscv.h>
#include <interrupt.hpp>

int main()
{
    kout<<"hello world"<<"\n";
    kout[red]<<"clock_init"<<endl;
    clock_init();
    kout[red]<<"trap_init"<<endl;
    trap_init();
    kout[red]<<"interrupt_enable"<<endl;
    interrupt_enable(); 

    
    while (1)
    {
        delay(100);
        kout<<'*';
    }
    return 0;
}