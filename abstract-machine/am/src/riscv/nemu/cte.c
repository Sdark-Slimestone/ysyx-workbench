#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <stdint.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  /*// 打印上下文内容，用于验证 Context 结构体顺序
  printf("========== Context Dump ==========\n");
  printf("mcause = 0x%x\n", c->mcause);
  printf("mstatus = 0x%x\n", c->mstatus);
  printf("mepc = 0x%x\n", c->mepc);
  printf("pdir = 0x%x\n", (uintptr_t)c->pdir);
  printf("gpr[0..31]:\n");
  for (int i = 0; i < 32; i++) {
    printf("  x%d = 0x%x%s", i, c->gpr[i], (i+1)%4 == 0 ? "\n" : "");
  }
  printf("==================================\n"); */

  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 8:
        if(c->gpr[17] == (uint32_t)-1)
          ev.event = EVENT_YIELD;
        else
          ev.event = EVENT_SYSCALL;
        break;
      default: ev.event = EVENT_ERROR; break;
    }
    c = user_handler(ev, c);
    assert(c != NULL);
  }
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
