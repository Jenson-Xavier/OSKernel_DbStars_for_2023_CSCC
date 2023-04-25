#include <pmm.hpp>
#include <kout.hpp>

void PMM::Init(uint64 _start, uint64 _end)
{
    all_pages = (PAGE *)_start;
    page_num = (_end - _start) / PAGESIZE;

    // 头节点设置
    page_used.addr = -1;
    page_used.num = 1;
    page_used.next = 0;
    page_used.ID = 0;

    // 内存表所占用内存
    all_pages[page_used.next].next = -1;
    all_pages[page_used.next].addr = _start;
    all_pages[page_used.next].ID = 0;
    all_pages[page_used.next].num = (page_num * sizeof(PAGE)) / PAGESIZE + 1;

    // 空闲页链表头节点设置
    page_free.addr = -1;
    page_free.num = 1;
    page_free.next = page_used.num;
    page_free.ID = -1;

    show(&page_used);
    show(&page_free);
}

void PMM::show(PAGE *pages)
{
    while (pages->next != -1)
    {
        pages=&all_pages[pages->next];
        kout << "addr " << pages->addr << endl;
        kout << "num  " << pages->num << endl;
        kout << "ID   " << pages->ID << endl;
        kout << "next " << pages->next << endl
             << endl;
        
    }
}

PAGE *PMM::alloc_pages(uint64 num)
{
}

PMM pmm;