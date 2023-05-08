#include <vmm.hpp>

PAGETABLE::PAGETABLE(uint64 addr,bool _IsKernelTable)
{
    IsKernelTable=_IsKernelTable;
    if (_IsKernelTable)
    {
        entries=boot_sv39_page_table; 
    }
    else
    {
        entries=(ENTRY *)pmm.alloc_pages(1);
    }
    
    
}