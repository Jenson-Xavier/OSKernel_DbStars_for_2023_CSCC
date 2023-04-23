#include <interrupt.hpp>
#include <Riscv.h>

void interrupt_enable()
{
    write_csr(sstatus,SSTATUS_SIE);
}
void interrupt_disable()
{
    clear_csr(sstatus,SSTATUS_SIE);
}