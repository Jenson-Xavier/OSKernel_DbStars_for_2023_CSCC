#ifndef __PMM_HPP__
#define __PMM_HPP__

// 先考虑基于QEMU模拟机上的运行 实际烧写等到具体场景再做修改
// QEMU模拟的DRAM物理内存大小
// QEMU模拟的物理内存地址从0x0到0x80000000处都是QEMU部分
// DRAM可分配的物理内存就是从0x80000000到0x88000000部分
// 默认物理内存大小是128MB 当然也是可以指定的 这里采取默认实现
#define MEMORYSTART 0x80200000
#define MEMORYSIZE 0x08000000 
#define MEMORYEND  0x88000000
#define PVOffset   0xffffffff00000000
// pagesize页大小为0x1000B 即4096B 即4KB标准sv39页大小
#define PAGESIZE   0x1000
#include <type.hpp>



struct PAGE
{
    PAGE* next;
    PAGE* pre;
    uint64 num;
};

extern "C"
{
    extern char kernel_end[];
};



// 双链表实现最优分配
class PMM
{

    // |====qemu=====|=====kernel===|==pmm==|===freepage==|
    // pmm部分是数组实现的双链表
    // 双链表方便处理
    // 一个链表维护已经使用过的页
    // 一个链表维护空闲待分配的页

private:
    PAGE  page_free; // 空闲页头节点
    PAGE  page_used; // 已分配页头节点
    PAGE* all_pages; // 记录所有页的地址 可以看成一个数组 代表的是数组的首地址 对应的就是页表基址
    uint64 page_num; // 总共可分配的页数
    uint64 end;      // 结束地址

    void show(PAGE* pages);                 // 显示链表中的内容
    bool insert_page(PAGE* src, PAGE* tar); // 将tar插入src链表中


public:
    void Init(uint64 _start = (uint64)kernel_end, uint64 _end = MEMORYEND+PVOffset);
    PAGE* alloc_pages(uint64 num);
    bool  free_pages(PAGE* t);
    PAGE* get_page_from_addr(void* addr);

    // 实现malloc和free
    // 基于已经实现的页分配简单实现
    // 实现并非很精细 没有充分处理碎片等问题 要求分配的空间必须为整数页张

    // 传参为需要分配的字节数 第二个默认参数为了适配页分配的_ID 基本不会使用
    // 其返回的参数就是实际内存块中的物理地址
    void* malloc(uint64 bytesize);
    void free(void* freeaddress);
};


extern PMM pmm;

//声明作为标准库通用的内存分配函数
inline void* kmalloc(uint64 bytesize)
{
    return pmm.malloc(bytesize);
}

inline void kfree(void* freeaddress)
{
    if (freeaddress != nullptr)
    {
        pmm.free(freeaddress);
    }
}

#endif