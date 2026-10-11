#include <am.h>

// ysyxSoC 的 CLINT @ 0x0200_0000: mtime (64 位, 每周期 +1) 在 +0xbff8/+0xbffc。
#define CLINT_BASE 0x02000000L

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint32_t lo = *(volatile uint32_t *)(CLINT_BASE + 0xbff8);
  uint32_t hi = *(volatile uint32_t *)(CLINT_BASE + 0xbffc);
  uptime->us = ((uint64_t)hi << 32) | lo;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
