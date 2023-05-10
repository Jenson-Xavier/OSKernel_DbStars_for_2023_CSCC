#include <interrupt.hpp>

void interrupt_enable()
{
    set_csr(sstatus, SSTATUS_SIE);
}

void interrupt_disable()
{
    clear_csr(sstatus, SSTATUS_SIE);
}