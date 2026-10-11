#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
#ifdef __riscv_e
      case 8:   // NEMU raw ecall (U-mode)
      case 11:  // M-mode ecall (NPC 运行在 M 态)
        if(c->gpr[15] == (uint32_t)-1)  // a5 = -1 (gpr[15] for rv32e)
          ev.event = EVENT_YIELD;
        else
          ev.event = EVENT_SYSCALL;
        break;
#else
      case 11:
        if(c->gpr[17] == (uint32_t)-1)  // a7 = -1
          ev.event = EVENT_YIELD;
        else
          ev.event = EVENT_SYSCALL;
        break;
#endif
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
  // 将 Context 结构放在栈顶（高地址）
  Context *c = (Context *)(kstack.end - sizeof(Context));
  // 清零整个结构（避免未初始化值，尤其是 mstatus）
  memset(c, 0, sizeof(Context));

  // 设置入口地址：trap.S 的 mret 前会 addi t2,t2,4，所以这里设 entry-4
  c->mepc = (uintptr_t)entry - 4;

  // 设置第一个参数 a0 = arg
  c->gpr[10] = (uintptr_t)arg;

  // 设置栈指针 sp = kstack.end（栈顶，栈向下增长）
  c->gpr[2] = (uintptr_t)kstack.end;

  // 设置 mstatus：riscv32 上下文 mstatus 设为 0x1800
  c->mstatus = 0x1800;

  return c;
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