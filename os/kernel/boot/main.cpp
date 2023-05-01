#include <sbi.h>
#include <kout.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <Riscv.h>
#include <interrupt.hpp>
#include <pmm.hpp>

int main()
{
    kout << "hello world!" << "\n";

    /*
    //test outHex
    uint64 xu64 = 123456789987654321;
    uint32 xu32 = 123456789;
    uint16 xu16 = 12345;
    uint8 xu8 = 123;
    kout << Hex(xu64) << endl;
    kout << Hex(xu32) << endl;
    kout << Hex(xu16) << endl;
    kout << Hex(xu8) << endl;
    */

    /*
    //test memory
    uint32 arr[] = { 1456467546, 2465464845, 345646546, 465454688, 546841385 };
    kout << Memory(arr, arr + 5, 4);
    */
    
    kout[red] << "clock_init" << endl;
    clock_init();
    kout[red] << "trap_init" << endl;
    trap_init();
    kout[red] << "interrupt_enable" << endl;
    interrupt_enable();
    pmm.Init();

    /*
    //test page alloc
    PAGE* a, * b, * c, * d, * e;
    kout[yellow] << 1 << endl;
    a = pmm.alloc_pages(2, 1);

    kout[yellow] << 2 << endl;
    b = pmm.alloc_pages(2, 1);

    kout[yellow] << 3 << endl;
    pmm.free_pages(a);

    kout[yellow] << 4 << endl;
    d = pmm.alloc_pages(3, 1);

    kout[yellow] << 5 << endl;
    e = pmm.alloc_pages(2, 1);
    */

    
    //test kmalloc and kfree
    void* testaddr = kmalloc(64);
    kout << Memory(testaddr, (void*)(testaddr + 64)) << endl;
    kout << Hex((uint64)testaddr) << endl;
    kfree(testaddr);
    
    
    while (1)
    {
        delay(100);
        kout << '*';
    }

    return 0;
}