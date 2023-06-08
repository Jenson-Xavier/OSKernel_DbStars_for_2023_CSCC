操作系统提供了对SV39的支持，用于将不同空间进行隔离。<br />虚拟内存管理由VMM(virtual memory manager)负责进行管理与调度。
```cpp
class VMS
{
private:
    static VMS *KernelVMS; // 用于管理内核的空间
    static VMS *CurVMS;

    VMR *Head;
    uint64 VMRCount;
    PAGETABLE *PDT;
    uint64 ShareCount;

public:
    void insert(VMR *tar);
    VMR *insert(uint64 start, uint64 end, uint32 flag);
    bool del(bool (*p)(VMR *tar));
    bool del(VMR *tar);
    VMR *find(void *addr);

    void ref(); // 和进程相关的引用解引用
    void unref();

    bool init();
    bool clear();   // 清除VMR List,清除前确保只有一个进程在使用该空间
    bool destroy(); // 删除VMS
	static bool Static_Init(); // 初始化内核空间时使用 
	void create_from_vms(VMS *vms);             // 进程间的拷贝需要 新建一个vms但是内容拷贝至其他的vms.需确保VMS为新建
    PAGETABLE *copy_PDT(PAGETABLE *src,int32 end); // i为0表示在根节点,end表示页表结尾

    void Enter(); // 将当前空间切换到该实例所表示的空间
    void Leave(); // 切换到内核空间

    void show();

    bool SolvePageFault(TRAPFRAME *tf);
}
```
其中PDT用于实际SV39的实现。当要使用对应空间时，先调用init进行初始化，随后insert对应空间。对应的会建立一个VMR链表，此链表用于管理虚拟空间的起始与属性。其中的静态VMS指针用于管理当前以及内核指针，当切换内存空间时则调用对应VMS的enter和leave。<br />VMR（virtual memory region）则作为虚拟空间的抽象层的基本单位，负责管理[start,end)间的内存，对应的属性与SV39对应。
```cpp
class VMR
{
    friend class VMS;

protected:
    VMR_FLAG flag;
    uint64 start, end; // 存储的是物理地址
    VMR *pre;
    VMR *next;
}

```
```cpp
enum VMR_flags
{
    VM_rw = 0b11,
    VM_rwx = 0b111,
    VM_kernel = 0b100111,
    VM_userstack = 0b1000001011,
    VM_userheap = 0b1000010011,
    VM_mmio = 0b10100011,
    VM_test = 0b1100011
};
union VMR_FLAG
{
    uint32 flag;
    struct
    {
        unsigned int Read : 1;
        unsigned int Write : 1;
        unsigned int Exec : 1;
        unsigned int Stack : 1;
        unsigned int Heap : 1;
        unsigned int Kernel : 1;
        unsigned int Shared : 1;
        unsigned int Device : 1;
        unsigned int File : 1;
        unsigned int Dynamic : 1;
        unsigned int reserve : 22; // 用于自己定制
    };
};

```
虚拟内存需要内存中的页表项进行管理，存在由虚拟地址到页表项，页表项到次级页表项，最终到物理页的过程。而ENTRY提供了这样的管理
```cpp
union ENTRY
{
    uint64 page_table_entry;

    // 用于直接对对应寄存器操作
    struct
    {
        unsigned int V : 1;
        unsigned int R : 1;
        unsigned int W : 1;
        unsigned int X : 1;
        unsigned int U : 1;
        unsigned int G : 1;
        unsigned int A : 1;
        unsigned int D : 1;
        unsigned int RSW : 2;
    };
}
```
而缺页中断则负责在发生缺页错误的时候调用pmm用于自动补全缺失的页表。<br />By：郭伟鑫
