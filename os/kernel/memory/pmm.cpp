#include <pmm.hpp>
#include <kout.hpp>

void PMM::Init(uint64 _start, uint64 _end)
{
    end = _end;
    all_pages = (PAGE*)_start;
    page_num = (_end - _start) / PAGESIZE;

    page_used.pre = nullptr;
    page_used.next = &all_pages[0];
    page_used.ID = -1;
    page_used.num = 1;
    all_pages[0].ID = 0;
    all_pages[0].next = nullptr;
    all_pages[0].pre = &page_used;
    all_pages[0].num = page_num * sizeof(PAGE) / PAGESIZE + 1; // 由于头节点存在，所以空闲两个页

    page_free.pre = nullptr;
    page_free.next = &all_pages[page_used.next->num];
    page_free.ID = -1;
    page_free.num = 1; // num表示后面有多少个页由这个链表头节点管理 因此可以视作扮演的是偏移量的角色
    all_pages[page_used.next->num].ID = 0;
    all_pages[page_used.next->num].next = nullptr;
    all_pages[page_used.next->num].pre = &page_free;
    all_pages[page_used.next->num].num = page_num - page_used.next->num;

    kout[green] << "PMM_init success" << endl;
    kout[green] << "page_used" << endl;
    show(&page_used); // test need
    kout[green] << "page_free" << endl;
    show(&page_free);
}

void PMM::show(PAGE* pages)
{
    PAGE* t = pages;
    while (t != nullptr)
    {
        kout << "ID  : " << t->ID << endl;
        kout << "num : " << t->num << endl;
        kout << "pre : " << KOUT::hex(uint64(t->pre)) << endl;
        kout << "next: " << KOUT::hex(uint64(t->next)) << endl;
        kout << "www : " << uint64(t - all_pages) << endl
            << endl;
        t = t->next;
    }
}

PAGE* PMM::alloc_pages(uint64 num, int _ID)
{
    // 从空闲页链表出发寻找可分配页
    PAGE* f = &page_free;
    while (f->next != nullptr)
    {
        if (f->next->num >= num)
        {
            PAGE* t;
            t = f->next;
            if (f->next->num == num)
            {
                f->next = t->next;
                if (t->next != nullptr)
                    t->next->pre = f;
                page_free.num--;
            }
            else
            {
                (t + num)->num = t->num - num;
                (t + num)->ID = t->ID;
                (t + num)->next = t->next;
                (t + num)->pre = t->pre;
                f->next = (t + num);
                if (t->next != nullptr)
                {
                    t->next->pre = (t + num);
                }
            }
            t->num = num;
            t->ID = _ID;

            if (insert_page(&page_used, t))
            {
                show(&page_used);
                kout << "-------" << endl;
                show(&page_free);
                kout[green] << "alloc memory success" << endl;
                return t;
            }
            else
            {
                kout[red] << "insert page error" << endl;
                return nullptr;
            }
        }

        f = f->next;
    }
    kout[red] << "alloc memory error" << endl;
    return nullptr;
}

bool PMM::insert_page(PAGE* src, PAGE* tar)
{
    src->num++;
    while (src != nullptr)
    {
        if (src->next > tar || src->next == nullptr)
        {
            tar->next = src->next;
            tar->pre = src;
            src->next = tar;
            if (tar->next != nullptr)
                tar->next->pre = tar;
            return true;
        }

        src = src->next;
    }
    return false;
}

bool PMM::free_pages(PAGE* t)
{
    if (t == nullptr)
    {
        return false;
    }

    t->pre->next = t->next;
    if (t->next != nullptr)
    {
        t->next->pre = t->pre;
    }
    if (insert_page(&page_free, t))
    {
        if ((t->pre->num) + (t->pre) == t && t->pre->ID != -1)
        {
            if (t->next)
                t->next->pre = t->pre;
            t->pre->next = t->next;
            t->pre->num = t->pre->num + t->num;
        }
        else if (t->next != nullptr && t->num + t == t->next)
        {
            if (t->next->next)
                t->next->next->pre = t;
            t->num = t->num + t->next->num;
            t->next = t->next->next;
        }

        page_used.num--;

        show(&page_used);
        kout << "-------" << endl;
        show(&page_free);
        kout[green] << "free page success" << endl;
        return true;
    }
    else
    {
        kout[red] << "free page error" << endl;
        return false;
    }
}

// 从物理地址获取对应的页 参数addr实际上是相对all_pages的偏移量
PAGE* PMM::get_page_from_addr(void* addr)
{
    return &all_pages[uint64(addr) / PAGESIZE];
}

// 实现malloc函数 返回的就是分配的内存块是实际物理地址
void* PMM::malloc(uint64 bytesize, int32 _id)
{
    int need_page_count = bytesize / PAGESIZE;
    if (bytesize % need_page_count != 0)
    {
        // 说明分配字节数不是整数倍
        // 强制整数 但是伴随产生内存碎片
        need_page_count++;
    }
    PAGE* temppage = alloc_pages(need_page_count, _id);
    if (temppage == nullptr)
    {
        // 没有分配成功
        kout[red] << "Malloc Fail!" << endl;
        return nullptr;
    }
    // 由于页分配实现的是返回页结构体指针 是个页表地址  
    // malloc实现的是返回成功分配后的内存起始地址
    return (void*)((uint64)(temppage - all_pages) * PAGESIZE + (uint64)all_pages);
}

void PMM::free(void* freeaddress)
{
    PAGE* wait_free_page = get_page_from_addr((void*)((uint64)freeaddress - (uint64)all_pages));

    if (wait_free_page == nullptr)
    {
        kout[red] << "Free Fail!" << endl;
        return;
    }
    if (!free_pages(wait_free_page))
    {
        kout[red] << "Free Fail!" << endl;
        return;
    }
    // Free Success
    return;
}

PMM pmm;