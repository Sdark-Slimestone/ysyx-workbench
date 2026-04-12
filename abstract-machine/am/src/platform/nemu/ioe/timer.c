#include <am.h>
#include <nemu.h>

static uint64_t boot_time = 0;          // 记录系统启动时的硬件时间

void __am_timer_init() {
    uint32_t high = inl(RTC_ADDR + 4);  //inl来自ysyx-workbench/abstract-machine/am/src/riscv/riscv.h
    uint32_t low  = inl(RTC_ADDR);      //rtcaddr来自ysyx-workbench/abstract-machine/am/src/platform/nemu/include/nemu.h
    boot_time = ((uint64_t)high << 32) | low;
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
    uint32_t high = inl(RTC_ADDR + 4);
    uint32_t low  = inl(RTC_ADDR);
    uint64_t now = ((uint64_t)high << 32) | low;
    uptime->us = now - boot_time;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
    // 实验不需要实现 RTC 日历，直接填默认值即可
    rtc->second = 0;
    rtc->minute = 0;
    rtc->hour   = 0;
    rtc->day    = 0;
    rtc->month  = 0;
    rtc->year   = 1900;
}