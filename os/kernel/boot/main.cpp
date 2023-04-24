#include <sbi.h>
#include <kout.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <Riscv.h>
#include <interrupt.hpp>

int main()
{
    kout << "hello world!" << "\n";

    /*
    //test outHex
    uint64 xu64 = 123456789987654321;
    uint32 xu32 = 123456789;
    uint16 xu16 = 12345;
    uint8 xu8 = 123;
    int64 x64 = -12345678987654321;
    int32 x32 = -123456789;
    int16 x16 = -12345;
    int8 x8 = -123;
    kout.hex(xu64) << endl;
    kout.hex(xu32) << endl;
    kout.hex(xu16) << endl;
    kout.hex(xu8) << endl;
    kout.hex(x64) << endl;
    kout.hex(x32) << endl;
    kout.hex(x16) << endl;
    kout.hex(x8) << endl;
    */

    kout[red] << "clock_init" << endl;
    clock_init();
    kout[red] << "trap_init" << endl;
    trap_init();
    kout[red] << "interrupt_enable" << endl;
    interrupt_enable();

    while (1)
    {
        delay(100);
        kout << '*';
    }
    
    return 0;
}