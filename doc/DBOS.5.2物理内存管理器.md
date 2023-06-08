在当前的操作系统中，物理内存管理的实现是由一个静态链表来完成分配的。页对象如下
```cpp
struct PAGE
{
    PAGE* next;
    PAGE* pre;
    uint64 num;
};
```
物理内存管理器通过硬编码确定物理地址的开始与结尾，将连续的空间看做4KB的结构体数组。使用首次适应算法，每次将最先找到的合适大小的空间分配出去。<br />虽然链表效率不高，但是我们还是对其进行了优化，用了两个链表进行管理，一个负责未分配的内存空间，另一个则负责占用了的内存空间。
```cpp
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
    void Init(uint64 _start = (uint64)kernel_end, uint64 _end = MEMORYEND + PVOffset);
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
```
主要提供两个接口，alloc_pages与free_pages。同时提供了kmalloc和kfree用于为其他应用的支持。<br />By：郭伟鑫
