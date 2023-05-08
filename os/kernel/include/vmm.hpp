#ifndef __VMM_HPP__
#define __VMM_HPP__

#include <type.hpp>
#include <pmm.hpp>


#define PhysiclaVirtualMemoryOffset 0xffffffff00000000
#define ENTRYCOUNT 512


class PAGETABLE;
extern "C"
{
    ENTRY boot_sv39_page_table[];
}

union ENTRY
{
    uint64 page_table_entry;
    struct // 用于直接对对应寄存器操作
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
    inline uint64 get_PNN()
    {
        return page_table_entry<<10>>20;
    }
    inline bool is_leaf()
    {
        return X+R+W;
    }
};

class PAGETABLE
{
private:
    ENTRY * entries;
    uint8 level;
    bool IsKernelTable;//判断页表是否在内核区
public:
    PAGETABLE(uint64 addr,bool _IsKernelTable);
    ~PAGETABLE();
};

#endif