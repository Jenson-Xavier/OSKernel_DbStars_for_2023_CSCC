#include <vmm.hpp>

VMS *VMS::KernelVMS = nullptr;
VMS *VMS::CurVMS = nullptr;

bool PAGETABLE::Init(bool isRoot)
{
    if (isRoot)
    {
        memset(entries, uint8(0), sizeof(ENTRY) * ENTRYCOUNT);
        memcpy(&entries[ENTRYCOUNT - 4], (char *)((ENTRY *)&boot_sv39_page_table[ENTRYCOUNT - 4]), sizeof(ENTRY) * 4);
    }
    else
    {
        memset(entries, uint8(0), sizeof(ENTRY) * ENTRYCOUNT);
    }
    return true;
}

bool PAGETABLE::Destroy()
{

    if (this == (void *)boot_sv39_page_table)
    {
        kout[red] << "can't Destroy Kernel PageTable" << endl;
        return false;
    }

    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if (entries[i].V && !entries[i].isKernelEntry())
        {
            if (entries[i].is_leaf())
            {
                pmm.free(entries[i].get_next_page());
            }
            else
            {
                entries[i].get_next_page()->Destroy();
            }
        }
        pmm.free(entries);
    }
    // kout[green] << "success Destroy" << endl;
    return true;
}

void PAGETABLE::show()
{
    if (entries == nullptr)
    {
        return;
    }

    kout[blue] << "--------------" << endl;
    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if (entries[i].V)
        {
            if (entries[i].is_leaf())
            {
                kout[blue] << "~~~~~~~~~~" << endl;
                kout[purple] << i << endl;
                kout[blue] << KOUT::hex(entries[i].get_PNN()) << endl;
                kout[blue] << "~~~~~~~~~~" << endl;
            }
            else
            {
                entries[i].get_next_page()->show();
            }
        }
    }
    kout[blue] << "--------------" << endl;
}

// bool PAGETABLE::del(uint64 start, uint64 end, uint8 level)
// {
//     uint64 a, b, h, e;
//     uint64 mask = 0x1ff;
//     a = ((start >> (level * 9 + 12)) & mask);
//     b = ((end >> (level * 9 + 12)) & mask);
//     if (a == b)
//     {
//         if (entries[a].V && !entries[a].is_leaf())
//         {
//             entries[a].get_next_page()->del((start >> ((level - 1) * 9 + 12)) & mask,)
//         }
//     }
// }

bool PAGETABLE::del(uint64 start, uint64 end, uint8 level)
{
    return true;
}

bool VMR::Init(uint64 _start, uint64 _end, uint32 _flags)
{
    start = _start;
    end = _end;
    flag.flag = _flags;
    pre = nullptr;
    next = nullptr;
    return true;
}

bool VMS::init()
{
    Head = nullptr;
    VMRCount = 0;

    PDT = (PAGETABLE *)pmm.malloc(4096);
    PDT->Init(1);

    ShareCount = 0;
    return true;
}

void VMS::insert(VMR *tar)
{
    // kout[green] << "success to insert" << endl;
    tar->pre = nullptr;
    tar->next = Head;
    if (Head != nullptr)
    {
        Head->pre = tar;
    }
    Head = tar;
}

VMR *VMS::insert(uint64 start, uint64 end, uint32 flag)
{
    // ... 需要进行对齐
    VMR *tar = (VMR *)pmm.malloc(sizeof(VMR));
    tar->start = start;
    tar->end = end;
    tar->flag.flag = flag;
    insert(tar);
    return tar;
}

void VMS::Leave()
{
    KernelVMS->Enter();
}

void VMS::Enter()
{
    if (CurVMS == this)
    {
        return;
    }
    CurVMS = this;
    lcr3((uint64)PDT->PAddr());
    asm volatile("sfence.vma \n fence.i \n fence");
    // kout[yellow] << "Enter VMS" << endl;
}

bool VMS::del(bool (*p)(VMR *tar))
{
    VMR *t, *t1;
    while (Head != nullptr && p(Head))
    {
        t = Head;
        pmm.free(t);
        Head = Head->next;
        VMRCount--;
        Head->pre = nullptr;
        PDT->del(t->start, t->end, 2);
    }

    if (Head == nullptr)
    {
        // kout[green] << "success del VMR" << endl;
        return true;
    }

    while (t->next != nullptr)
    {
        while (t->next && p(t->next))
        {
            t1 = t->next;
            t->next = t1->next;
            if (t1->next)
                t1->next->pre = t;
            pmm.free(t1);
            VMRCount--;
            PDT->del(t1->start, t1->end, 2);
        }
        t = t->next;
    }
    // kout[green] << "success del VMR" << endl;
    return true;
}

bool VMS::del(VMR *tar)
{
    if (tar->pre == nullptr && tar->next == nullptr)
    {
        Head = nullptr;
        return true;
    }
    if (tar->pre)
        tar->pre->next = tar->next;
    if (tar->next)
        tar->next->pre = tar->pre;
    return false;
}

VMR *VMS::find(void *addr)
{
    VMR *t = Head;
    while (t != nullptr)
    {
        if (t->GetStart() <= (uint64)addr && t->GetEnd() > (uint64)addr)
            return t;
        t = t->next;
    }
    return nullptr;
}

void VMS::ref()
{
    ShareCount++;
}

void VMS::unref()
{
    ShareCount--;
}

bool VMS::Static_Init()
{
    KernelVMS = (VMS *)pmm.malloc(sizeof(VMS));
    if (KernelVMS == nullptr)
    {
        kout[red] << "failed to malloc Kernel VMS" << endl;
        return false;
    }

    KernelVMS->PDT = (PAGETABLE *)boot_sv39_page_table;
    KernelVMS->VMRCount = 1;

    KernelVMS->Head = (VMR *)pmm.malloc(sizeof(VMR));
    KernelVMS->Head->Init(0xffffffff00000000, 0xffffffffc0000000, 39);
    KernelVMS->insert(PhysiclaVirtualMemoryOffset, PhysiclaVirtualMemoryOffset + 0xc0000000, 163);

    KernelVMS->ShareCount = 1;
    kout[blue] << "VirtualMemorySpace for Kernel init Success!" << endl;
    return true;
}

bool VMS::clear()
{
    if (ShareCount > 1)
    {
        kout[red] << "failed to clear VMS.More than one process use this VMS" << endl;
        return false;
    }
    else
    {
        if (Head != nullptr)
        {
            VMR *t;
            while (Head->next)
            {
                t = Head->next;
                pmm.free(t);
                Head->next = t->next;
            }
            pmm.free(Head);
            Head = nullptr;
        }
        PDT->Destroy();
    }
    ShareCount = 1;

    // kout[green] << "success clear VMS" << endl;
    return true;
}

bool VMS::destroy()
{
    if (!this->clear())
    {
        return false;
    }
    pmm.free(PDT);
    pmm.free(this);
    return true;
}

void VMS::create_from_vms(VMS *vms)
{
    // 复制VMR
    if (Head != nullptr)
    {
        kout[red] << "vms is not empty" << endl;
        return;
    }

    VMR *t = vms->Head;
    while (t)
    {
        this->insert(t->start, t->end, t->flag.flag);
        t = t->next;
    }

    VMRCount = vms->VMRCount;
    ShareCount = vms->ShareCount;

    // 复制PDT
    PDT = copy_PDT(vms->PDT, 0);
}

PAGETABLE *VMS::copy_PDT(PAGETABLE *pdt, int i)
{
    if (!i)
    {
        pmm.free(PDT);
    }

    if (i == 3)
    {
        PAGETABLE *t = (PAGETABLE *)pmm.malloc(PAGESIZE);
        memcpy(t, (char *)pdt, PAGESIZE);
        return t;
    }

    PAGETABLE *re = (PAGETABLE *)pmm.malloc(PAGESIZE);
    PAGETABLE *t;

    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if (pdt->getEntry(i).V)
        {
            t = copy_PDT(pdt->getEntry(i).get_next_page(), i + 1);
            re->getEntry(i).set_PNN(t);
        }
    }
    return re;
}

void VMS::show()
{
    VMR *t = Head;
    while (t)
    {
        kout[blue] << "start :" << KOUT::hex(t->GetStart()) << endl;
        kout[blue] << "end   :" << KOUT::hex(t->GetEnd()) << endl;
        kout[blue] << "flag  :" << KOUT::hex(t->GetFlags().flag) << endl;
        t = t->next;
    }
    kout << endl;
}

bool VMS::SolvePageFault(TRAPFRAME *tf)
{
    VMR *t = find((void *)(tf->badvaddr));
    if (t == nullptr)
    {
        kout[red] << "Invalid Addr : ";
        kout[red] << Hex(tf->badvaddr) << endl;
        return false;
    }

    ENTRY &e2 = PDT->getEntry(((tf->badvaddr >> 12) >> (9 * 2)) & 511);
    PAGETABLE *p;
    if (!e2.V)
    {
        p = (PAGETABLE *)pmm.malloc(4096);
        p->Init();
        e2.set_PNN(p->PAddr());
        // kout[yellow] << KOUT::hex(e2.get_PNN()) << endl;
        e2.V = 1;
        e2.W = 0;
        e2.R = 0;
        e2.X = 0;
    }
    p = e2.get_next_page();

    ENTRY &e1 = p->getEntry(((tf->badvaddr >> 12) >> (9 * 1)) & 511);
    if (!e1.V)
    {
        p = (PAGETABLE *)pmm.malloc(4096);
        p->Init();
        e1.set_PNN(p->PAddr());
        e1.V = 1;
        e1.W = 0;
        e1.R = 0;
        e1.X = 0;
    }
    p = e1.get_next_page();

    ENTRY &e0 = p->getEntry((tf->badvaddr >> 12) & 511);
    // kout.memory(&e0, 8);
    // kout[yellow] << (uint64)p << endl;
    if (!e0.V)
    {
        p = (PAGETABLE *)pmm.malloc(4096);
        e0.set_PNN(p->PAddr());
        e0.V = 1;
        e0.W = t->flag.Write;
        e0.R = t->flag.Read;
        e0.X = t->flag.Exec;
        if (t->flag.Kernel)
            e0.U = 0;
        else
            e0.U = 1; // 用户页面需要增加用户标志位
    }
    else
    {
        kout[red] << "page exists but still error" << endl;
        return false;
    }

    asm volatile("sfence.vma \n fence.i \n fence");
    return true;
}

bool trap_PageFault(TRAPFRAME *tf)
{
    return VMS::GetCurVMS()->SolvePageFault(tf);
}

void *operator new(size_t size)
{
    return pmm.malloc(size);
}

void *operator new[](size_t size)
{
    return pmm.malloc(size);
}

void operator delete(void *p, size_t size)
{
    pmm.free(p);
}

void operator delete[](void *p)
{
    pmm.free(p);
}