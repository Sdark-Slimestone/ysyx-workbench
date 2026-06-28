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
    assert(c != NULL);   // <-----33
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

Context* kcontext(Area kstack, void (*entry)(void *), void *arg) {
  // 将 Context 结构放在栈顶（高地址）
  Context *c = (Context *)(kstack.end - sizeof(Context));
  // 清零整个结构（避免未初始化值，尤其是 mstatus）
  memset(c, 0, sizeof(Context));

  // 设置入口地址
  c->mepc = (uintptr_t)entry;

  // 设置第一个参数 a0 = arg
  c->gpr[10] = (uintptr_t)arg;

  // 设置栈指针 sp = kstack.end（栈顶，栈向下增长）
  c->gpr[2] = (uintptr_t)kstack.end;

  // 设置 mstatus：使能全局中断，并设置 MPP 为 U 态（若需要内核线程运行在 M 态，可设 0x1800）
  // PA4 中，内核线程仍在 M 态，所以 mstatus 的 MPP 域（bit12-11）通常设为 0b11（M 态）。
  // 参考 RISC-V 特权级：mstatus.MPP = 11 表示返回 M 态，MIE = 1 使能中断。
  // 你可以使用 0x1880 这个常见值（MPIE=1, MPP=00? 需要查手册）。
  // 最简单：从当前异常保存的 mstatus 复制（通过读取 csr）或者直接写一个安全值。
  // 这里给出一个合理的初始值：
  c->mstatus = 0x1880;   // MIE=1, MPIE=1, MPP=00（实际取决于实现，暂不影响）

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
