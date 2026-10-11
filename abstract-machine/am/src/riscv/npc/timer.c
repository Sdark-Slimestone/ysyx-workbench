#include <am.h>

// CLINT 的 mtime 每周期 +1 (单位是"周期")。仿真环境没有主频概念, 需要乘一个系数把它
// 换算成真实时间。按讲义: 让系数等于**仿真速率**(仿真器每秒执行的周期数), 这样
// AM_TIMER_UPTIME 返回的 us 就接近真实 (墙钟) 时间。
// 实测本核在 Verilator 下的执行速率约 0.6 M cycles/s (随程序/机器略有浮动), 取 600000。
#define CLINT_FREQ_HZ 600000

#define CLINT_MTIME_LO 0xa0000048   // mtime 低 32 位
#define CLINT_MTIME_HI 0xa000004c   // mtime 高 32 位

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
    // 32 位 NPC 需依次读 mtime 的低/高 32 位拼成 64 位。
    // 读期间 mtime 仍在增长, 用"重读高 32 位确认未进位"的方式得到一致的 64 位值。
    uint32_t hi, lo, hi2;
    do {
        hi  = *(volatile uint32_t *)CLINT_MTIME_HI;
        lo  = *(volatile uint32_t *)CLINT_MTIME_LO;
        hi2 = *(volatile uint32_t *)CLINT_MTIME_HI;
    } while (hi != hi2);
    uint64_t mtime = ((uint64_t)hi << 32) | lo;
    uptime->us = mtime * 1000000ull / CLINT_FREQ_HZ;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
    rtc->second = 0;
    rtc->minute = 0;
    rtc->hour   = 0;
    rtc->day    = 0;
    rtc->month  = 0;
    rtc->year   = 1900;
}
