#ifndef __CLOCK_HPP__
#define __CLOCK_HPP__

#include "type.hpp"

    typedef uint64  clock_t;
    const  clock_t tickDuration = 1e4;
    extern volatile clock_t tickCount;


    void set_next_tick();
    clock_t get_clock_time();
    void clock_init();
    void delay(uint64 t);

#endif