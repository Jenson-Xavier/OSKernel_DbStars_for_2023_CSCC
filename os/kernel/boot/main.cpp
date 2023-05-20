#include <sbi.h>
#include <Riscv.h>
#include <klib.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <interrupt.hpp>
#include <memory.hpp>
#include <process.hpp>
#include <synchronize.hpp>
#include <resources.hpp>

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
    a = pmm.alloc_pages(2);

    kout[yellow] << 2 << endl;
    b = pmm.alloc_pages(2);

    kout[yellow] << 3 << endl;
    pmm.free_pages(a);

    kout[yellow] << 4 << endl;
    d = pmm.alloc_pages(3);

    kout[yellow] << 5 << endl;
    e = pmm.alloc_pages(2);
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
    // int test_array[25];
    // memset(test_array, 0, sizeof test_array);
    // for (int i = 0;i < 25;i++) {
    //     kout << test_array[i] << ' ';
    // }
    char testget[30];
    gets(testget, 20);
    puts(testget);
}

void test_process1()
{
    // pm.Init();
    // proc_struct* current = pm.get_cur_proc();

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
    pm.set_proc_name(test_proc, (char*)"User_Test_Process");
    pm.set_proc_kstk(test_proc, nullptr, KERNELSTACKSIZE * 4);
    pm.set_proc_vms(test_proc, VMS::GetKernelVMS());
    pm.show(test_proc);
    VMS::GetKernelVMS()->show();
    // pm.show(test_proc);
    // int testid = test_proc->pid;
    // proc_struct* tm = pm.get_proc(testid);
    // pm.show(tm);
    // pm.print_all_list();
    // proc_struct* test2 = pm.alloc_proc();
    // pm.init_proc(test2, 1);
    // pm.set_proc_name(test2, (char*)"User_Test_Process");
    // pm.set_proc_kstk(test2, nullptr, KERNELSTACKSIZE);
    // pm.set_proc_vms(test2, VMS::GetKernelVMS());
    // pm.print_all_list();
    // kout[red] << pm.get_proc_count() << endl;
    // pm.free_proc(test2);
    // // pm.print_all_list();
    // kout[red] << pm.get_proc_count() << endl;
    // // pm.free_proc(idle_proc);
    // pm.free_proc(test_proc);
    // kout[red] << pm.get_proc_count() << endl;
    // pm.print_all_list();
}

void test_process2()
{
    auto func = [](void* data)->int
    {
        const char* str = (const char*)data;
        // if (str[0] == '#')
        // {
        //     pm.get_cur_proc()->vms->insert(0x123, 0x456, 7);
        // }
        while (*str)
        {
            int n = 1e8;
            while (n-- > 0);
            kout << *str++;
            // if ((*str) == '#')pm.rest_proc(pm.get_cur_proc());
        }
        // pm.show(pm.get_cur_proc());
        return 0;
    };
    CreateKernelProcess(func, (void*)"########", (char*)"test1");
    CreateKernelProcess(func, (void*)"%%%%%%%%", (char*)"test2");
    // CreateKernelProcess(func, (void*)"********", (char*)"test3");
}

void test_lock()
{
    auto func = [](void* data)->int
    {
        int k = 0;                   // 测试时需要移到全局位置
        int last = 0;
        const char* str = (const char*)data;
        if (!mutex.try_mutex())
        {
            mutex.release_mutex();
            mutex.enter_mutex();
        }
        // if (!spin_lock.try_lock())
        // {
        //     spin_lock.unlock();
        //     spin_lock.lock();
        // }
        for (int j = 0;j < 10;j++)
        {
            k++;
            int n = 1e8;
            while (n-- > 0);
            // kout << str << ':' << k << " lock: " << spin_lock.get_slv() << endl;
            kout << str << ':' << k << " belong pid: " << mutex.get_blid() << endl;
        }
        mutex.release_mutex();
        // spin_lock.unlock();
        return 0;
    };
    CreateKernelProcess(func, (void*)"test1", (char*)"test1");
    CreateKernelProcess(func, (void*)"test2", (char*)"test2");
    CreateKernelProcess(func, (void*)"test3", (char*)"test3");
}

void test_processqueue()
{
    proc_queue q;
    pqm.init(q);

    proc_struct* test1 = pm.alloc_proc();
    pm.init_proc(test1, 1);
    proc_struct* test2 = pm.alloc_proc();
    pm.init_proc(test2, 2);

    kout << pqm.length_pq(q) << endl;
    pqm.enqueue_pq(q, test1);
    pm.show(pqm.front_pq(q));
    pqm.enqueue_pq(q, test2);
    pm.show(pqm.front_pq(q));
    kout << pqm.length_pq(q) << endl;
    pqm.print_all_queue(q);
    pqm.dequeue_pq(q);
    pqm.dequeue_pq(q);
    pqm.dequeue_pq(q);
    pqm.print_all_queue(q);
    kout << pqm.length_pq(q) << endl;
    pm.print_all_list();
}

void test_semaphore()
{
    auto func = [](void* data)->int
    {
        int get = semaphore.wait();
        if (get < 0)pm.imme_trigger_schedule();
        const char* str = (const char*)data;
        while (*str)
        {
            int n = 1e8;
            while (n-- > 0);
            kout << *str++;
        }
        semaphore.signal();
        return 0;
    };
    semaphore.init(2);
    CreateKernelProcess(func, (void*)"AAAAAAAAAAAAAAAAAAAAAA", (char*)"test1");
    CreateKernelProcess(func, (void*)"BBBBBBBBBBBBBBBBBBBBBB", (char*)"test2");
    CreateKernelProcess(func, (void*)"CCCCCCCCCCCCCCCCCCCCCC", (char*)"test3");
}

void test_pageTable()
{
    PAGETABLE* A, * B;
    A = (PAGETABLE*)pmm.malloc(sizeof(PAGETABLE));
    kout[yellow] << 1 << endl;
    A->Init(1);
    A->show();
    A->Destroy();

    kout[yellow] << 2 << endl;
    A = (PAGETABLE*)pmm.malloc(sizeof(PAGETABLE));
    A->Init(0);
    A->show();
    A->Destroy();

    kout[yellow] << 3 << endl;
    A = (PAGETABLE*)pmm.malloc(sizeof(PAGETABLE));
    B = (PAGETABLE*)pmm.malloc(sizeof(PAGETABLE));
    A->Init(0);
    B->Init();
    A->getEntry(3) = ENTRY::arr_to_ENTRY(P2KAddr(&B[0]));
    A->show();
    A->Destroy();
}

void test_VMS()
{
    VMS::Static_Init();
    VMS::GetKernelVMS()->show();
    VMS a;
    a.init();
    VMR A, B;
    A.Init(0x123, 0x456, 7);
    B.Init(0x456, 0x789, 7);
    a.insert(&A);
    a.insert(&B);
    a.show();
    kout << "-----" << endl;
    a.del(&A);
    a.show();
}

void test_page_fault()
{
    VMS A;
    A.init();
    VMR a;
    a.Init(PAGESIZE, 512 * PAGESIZE, 7);
    A.insert(&a);
    A.show();
    A.GetPageTable()->show();

    A.Enter();
    VMS::GetCurVMS()->show();

    char* str = (char*)PAGESIZE;
    strcpy(str, "hello");
    // *str='1';
    kout[yellow] << 1 << endl;

    A.show();
    A.GetPageTable()->show();
    kout[green] << str << endl;
}

void test_user_img_process()
{
    uint64 ts = (uint64)get_resource_begin(test_img);
    uint64 te = (uint64)get_resource_end(test_img);
    kout << Hex(ts) << ' ' << Hex(te) << endl;
    CreateUserImgProcess(ts, te, (char*)"u_test");
}

int main()
{
    kout << endl;
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
    VMS::Static_Init();
    pm.Init();

    test_user_img_process();
    
    while (1)
    {
        delay(100);
        // kout << '*';
    }

    return 0;
}