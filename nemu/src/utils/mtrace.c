#include <common.h>
#include "mtrace.h"

#ifdef CONFIG_MTRACE

// 输出一条访存记录
static void output_mtrace(const char *op, paddr_t addr, int len, word_t data) {
    printf("[mtrace] %s addr=0x%08x, len=%d, data=0x%08x\n",
           op, addr, len, data);
}

void mtrace_record_read(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE_COND
    // 在这里手写过滤条件，例如只关心 0x80000000~0x8000ffff
    // if (!(addr >= 0x80000000 && addr <= 0x8000ffff)) return;
#endif
    output_mtrace("READ ", addr, len, data);
}

void mtrace_record_write(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE_COND
    // 同上，按需修改条件
    // if (!(addr >= 0x80000000 && addr <= 0x8000ffff)) return;
#endif
    output_mtrace("WRITE", addr, len, data);
}

#endif