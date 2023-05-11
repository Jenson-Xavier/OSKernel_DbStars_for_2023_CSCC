#ifndef __VMM_HPP__
#define __VMM_HPP__

#include <type.hpp>
#include <pmm.hpp>
#include <klib.hpp>
#include <process.hpp>
#include <trap.hpp>
#include <Riscv.h>

#define PhysiclaVirtualMemoryOffset 0xffffffff00000000
#define ENTRYCOUNT 512

// 区分三种地址：
//  1、物理地址:只有ENTRY和satp中使用
//  2、内核地址:物理内存管理器返回的都是内核地址
//  3、虚拟地址:进入用户线程之后的地址

inline void* K2PAddr(void* Kaddr)
{
    return (void*)((uint64)Kaddr + PhysiclaVirtualMemoryOffset);
}

inline void* P2KAddr(void* Paddr)
{
    return (void*)(((uint64)(Paddr)) - PhysiclaVirtualMemoryOffset);
}

class VMS;

class PAGETABLE;

union ENTRY;

extern "C"
{
    extern ENTRY boot_sv39_page_table[];
}

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

    // 物理地址
    static inline ENTRY arr_to_ENTRY(void* addr, uint32 Res = 15)
    {
        ENTRY re;
        re.page_table_entry = ((uint64(addr) >> 12) << 10) + Res;
        return re;
    }

    inline uint64 get_PNN()
    {
        return (page_table_entry << 10) >> 20;
    }

    inline void set_PNN(void* page_PAddr)
    {
        page_table_entry = (((uint64)page_PAddr >> 12) << 10) | (page_table_entry & ((1 << 10) - 1));
    }

    inline bool is_leaf()
    {
        return X + R + W;
    }

    // 返回内核地址
    inline PAGETABLE* get_next_page()
    {
        if (is_leaf())
            return nullptr;
        else
            return (PAGETABLE*)((get_PNN() << 12) + PhysiclaVirtualMemoryOffset);
    }

    inline bool isKernelEntry()
    {
        if (page_table_entry == (0x00000ull << 10) | 0xCF || page_table_entry == (0x40000ull << 10) | 0xCF || page_table_entry == (0x80000ull << 10) | 0xCF || page_table_entry == (0xc0000ull << 10) | 0xCF)
            return true;
        else
            return false;
    }
};

class PAGETABLE
{
private:
    ENTRY entries[ENTRYCOUNT];

public:
    bool Init(bool isRoot = false);
    bool Destroy();                                     // 销毁之后只是物理内存将其释放

    void show();
    bool del(uint64 start, uint64 end, uint8 level);    // 必须和页框对齐,实现遇到困难，暂时停止

    inline void* PAddr()                                // 因为物理内存管理器不通过sv39返回
    {
        return (void*)((uint64)this - PhysiclaVirtualMemoryOffset);
    }

    inline ENTRY& getEntry(uint32 index)
    {
        return entries[index];
    }
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
        unsigned int reserve : 22;                      // 用于自己定制
    };
};

class VMR
{
    friend class VMS;
private:
    VMR_FLAG flag;
    uint64 start, end;                                  // 存储的是物理地址
    VMR* pre, * next;

public:
    inline uint64 GetLength()
    {
        return start - end;
    }

    inline uint64 GetStart()
    {
        return start;
    }

    inline uint64 GetEnd()
    {
        return end;
    }

    inline union VMR_FLAG GetFlags()
    {
        return flag;
    }

    bool Init(uint64 _start, uint64 _end, uint32 _flags);
};

// 每个线程中都要拥有一个VMS实例用于管理线程的空间
class VMS
{
private:
    static VMS* KernelVMS;                              // 用于管理内核的空间
    static VMS* CurVMS;

    VMR* Head;
    uint64 VMRCount;
    PAGETABLE* PDT;
    uint64 ShareCount;

public:
    void insert(VMR* tar);
    VMR* insert(uint64  start, uint64 end, uint32 flag);
    bool del(bool (*p)(VMR* tar));
    bool del(VMR* tar);
    VMR* find(void* addr);

    void ref();                             // 和进程相关的引用解引用
    void unref();
    
    bool init();
    bool clear();                           // 清除VMR List,清除前确保只有一个进程在使用该空间
    bool destroy();                         // 删除VMS

    static bool Static_Init();              // 初始化内核空间时使用

    inline static VMS* GetKernelVMS()
    {
        return KernelVMS;
    }

    inline static VMS* GetCurVMS()
    {
        return CurVMS;
    }

    inline PAGETABLE* GetPageTable()
    {
        return PDT;
    }

    inline uint64 GetShareCount()
    {
        return ShareCount;
    }

    void Enter();                           // 将当前空间切换到该实例所表示的空间
    void Leave();                           // 切换到内核空间

    void show();

    bool SolvePageFault(TRAPFRAME* tf);
};

bool TrapFunc_PageFault(TRAPFRAME* tf);

#endif