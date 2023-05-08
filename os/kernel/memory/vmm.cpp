#include <vmm.hpp>

bool PAGETABLE::Init(uint8 _level, bool _IsKernelTable)
{
    IsKernelTable = _IsKernelTable;
    level = _level;
    if (_IsKernelTable)
    {
        entries = boot_sv39_page_table;
    }
    else
    {
        entries = (ENTRY *)pmm.malloc(4096);
        if (entries==nullptr)
        {
            return false;
        }
        memset(entries, uint8(0), sizeof(ENTRY) * ENTRYCOUNT);
    }
    return true;
}


bool PAGETABLE::Destroy()
{
    if (IsKernelTable)
    {
        return false;        
    }

    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if (entries[i].V)
        {
            if (entries[i].is_leaf())
                pmm.free(entries[i].get_next_page());
            else
                entries[i].get_next_page()->Destroy(); 
        }
        pmm.free(entries);
    }
    return true;
}

void PAGETABLE::show()
{
    if (entries==nullptr)
    {
        return;
    }
    
    kout[blue]<<"--------------"<<endl;
    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if(entries[i].V)
        {
            if (entries[i].is_leaf())
            {
                kout[blue]<<"~~~~~~~~~~"<<endl;
                kout[blue]<<entries[i].get_PNN()<<endl;
                kout[blue]<<"~~~~~~~~~~"<<endl;
            }
            else
            {
                entries[i].get_next_page()->show();
            }
        }
    }
    kout[blue]<<"--------------"<<endl;
}