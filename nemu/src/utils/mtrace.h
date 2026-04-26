#ifndef __MTRACE_H__
#define __MTRACE_H__

#include <common.h>

#ifdef CONFIG_MTRACE
void mtrace_record_read(paddr_t addr, int len, word_t data);
void mtrace_record_write(paddr_t addr, int len, word_t data);
#else
static inline void mtrace_record_read(paddr_t addr, int len, word_t data) {}
static inline void mtrace_record_write(paddr_t addr, int len, word_t data) {}
#endif

#endif