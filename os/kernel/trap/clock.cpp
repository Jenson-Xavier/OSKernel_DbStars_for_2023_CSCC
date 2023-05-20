#include <clock.hpp>
#include <sbi.h>
#include <Riscv.h>
#include <kout.hpp>

volatile clock_t tickCount;

clock_t get_clock_time()
{
    return sbi_get_time();
}

void set_next_tick()
{
    sbi_set_time(sbi_get_time() + tickDuration);
}

void clock_init()
{
    set_csr(sie, MIP_STIP);
    tickCount = 0;
    set_next_tick();
}

void delay(uint64 t)
{
    while (t-- > 0)
    {
        int i = 1000;
        while (i-- > 0);
    }
}

void imme_trigger_timer()
{
    clock_t imme_duration = 1e3;                    // 在qemu上1e3即0.1ms 可以认为是立即触发的
    sbi_set_time(sbi_get_time());                   // 0.1ms 立即触发了时钟中断
}