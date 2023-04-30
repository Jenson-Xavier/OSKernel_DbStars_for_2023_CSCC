#ifndef __PMM_HPP__
#define __PMM_HPP__

#define MEMORYSIZE 0x08000000
#define MEMORYEND  0x88000000
#define PAGESIZE   0x1000
#include<type.hpp>



struct PAGE
{
    PAGE * next;
    PAGE * pre;
    uint64 num;
    int ID;
};

extern"C"
{
    extern char kernel_end[];
};



class PMM//双链表实现最优分配
{

    //|====qemu=====|=====kernel===|==pmm==|===freepage==|
    //pmm部分是数组实现的双链表
private:
    PAGE  page_free;//头节点
    PAGE  page_used;
    PAGE* all_pages;
    uint64 page_num;
    uint64 end;

    void show(PAGE* pages);//显示链表中的内容
    bool insert_page(PAGE* src, PAGE* tar);//将tar插入src链表中


public:
    void Init(uint64 _start = (uint64)kernel_end, uint64 _end = MEMORYEND);
    PAGE* alloc_pages(uint64 num,int _ID);
    bool  free_pages(PAGE * t);
    PAGE* get_page_from_addr(void* addr);

};


extern PMM pmm;

#endif