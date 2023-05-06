#include <sbi.h>
#include <kout.hpp>
#include <kstring.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <Riscv.h>
#include <interrupt.hpp>
#include <pmm.hpp>
#include <process.hpp>

void test_outHex()
{
    // test outHex
    uint64 xu64 = 123456789987654321;
    uint32 xu32 = 123456789;
    uint16 xu16 = 12345;
    uint8 xu8 = 123;
    kout << Hex(xu64) << endl;
    kout << Hex(xu32) << endl;
    kout << Hex(xu16) << endl;
    kout << Hex(xu8) << endl;
}

void test_memory()
{
    // test memory
    uint32 arr[] = { 1456467546, 2465464845, 345646546, 465454688, 546841385 };
    kout << Memory(arr, arr + 5, 4);
}

void test_page_alloc()
{
    // test page alloc
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
}

void test_kmalloc()
{
    // test kmalloc and kfree
    void* testaddr = kmalloc(64);
    // kout << Memory(testaddr, (void*)(testaddr + 64)) << endl;
    kout << Hex((uint64)testaddr) << endl;
    kfree(testaddr);
}

void test_kstring()
{
    int test_array[25];
    // memset(test_array, 0, sizeof test_array);
    for (int i = 0;i < 25;i++) {
        kout << test_array[i] << ' ';
    }
}

void test_process1()
{
    pm.Init();
    proc_struct* current = pm.get_cur_proc();

    // 输出调试信息
    // kout << Hex((uint64)idle_proc) << endl
    //     << "idle_proc's name : " << idle_proc->name << endl
    //     << "cur_proc's name : " << current->name << endl
    //     << "cur_proc's pid : " << current->pid << endl
    //     << "idle_proc's pid : " << idle_proc->pid << endl
    //     << "idle_proc's state : " << idle_proc->stat << endl
    //     << "idle_proc's kernelstackaddr : " << Hex((uint64)idle_proc->kstack) << endl
    //     << "idle_proc's kernelstacksize : " << idle_proc->kstacksize << endl
    //     << "idle_proc's kernel_user flag : " << idle_proc->ku_flag << endl
    //     << "ProcessManager Init Successfully!" << endl;

    proc_struct* test_proc = pm.alloc_proc();
    // pm.show(test_proc);
    pm.init_proc(test_proc, 1);
    // pm.show(test_proc);
    int testid = test_proc->pid;
    proc_struct* tm = pm.get_proc(testid);
    // pm.show(tm);
    // pm.print_all_list();
    proc_struct* test2 = pm.alloc_proc();
    pm.init_proc(test2, 2);
    pm.set_proc_name(test2, (char*)"User_Test_Process");
    pm.set_proc_kstk(test2, nullptr, KERNELSTACKSIZE);
    // pm.print_all_list();
    kout[red] << pm.get_proc_count() << endl;
    pm.free_proc(test2);
    // pm.print_all_list();
    kout[red] << pm.get_proc_count() << endl;
    // pm.free_proc(idle_proc);
    pm.free_proc(test_proc);
    kout[red] << pm.get_proc_count() << endl;
    pm.print_all_list();
}

void test_process2()
{
    auto func = [](void* data)->int
    {
        const char* str = (const char*)data;
        while (*str)
        {
            int n = 1e8;
            while (n-- > 0);
            kout << *str++;
        }
        return 0;
    };
    CreateKernelProcess(func, (void*)"###OP###HIJKLMN###", (char*)"test1");
    CreateKernelProcess(func, (void*)"%%%QR%%%%ABCDEFG%%", (char*)"test2");
    CreateKernelProcess(func, (void*)"(((UVWXYZ((())ST))", (char*)"test3");
}

int main()
{
    kout[purple] << "Hello World,OS Kernel!" << endl;
    kout << endl;

    kout[red] << "clock_init!" << endl;
    clock_init();
    kout[red] << "trap_init!" << endl;
    trap_init();
    kout[red] << "interrupt_enable!" << endl;
    interrupt_enable();
    kout << endl;

    pmm.Init();
    pm.Init();

    
    while (1)
    {
        delay(100);
        // kout << '*';
    }

    return 0;
}