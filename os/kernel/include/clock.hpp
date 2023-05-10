#ifndef __CLOCK_HPP__
#define __CLOCK_HPP__

#include "type.hpp"

typedef uint64  clock_t;

const clock_t timer_1ms = 1e4;              // QEMU上 时钟频率1e7Hz 1个时钟中断1ms->1e4
const clock_t timer_10ms = timer_1ms * 10;
const clock_t timer_100ms = timer_1ms * 100;
const clock_t timer_1s = timer_1ms * 1000;

const  clock_t tickDuration = timer_10ms;   // 用10ms作为一次时钟中断周期 也就是每秒触发100次时钟中断
extern volatile clock_t tickCount;


void set_next_tick();
clock_t get_clock_time();
void clock_init();
void delay(uint64 t);

void imme_trigger_timer();                  // 考虑进程调度 实现一个立即触发时钟中断的函数 注意应该只触发一次

#endif