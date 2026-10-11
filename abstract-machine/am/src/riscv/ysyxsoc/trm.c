#include <am.h>
#include <klib-macros.h>

extern char _heap_start;
extern char _stack_top;
int main(const char *args);

// 堆区: [SRAM 中 .bss 之后, 栈顶)。栈在 SRAM 末尾。
Area heap = RANGE(&_heap_start, &_stack_top);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER);

// ysyxSoC 的 UART16550 @ 0x1000_0000 (寄存器按 1 字节间隔)
#define UART_BASE 0x10000000L
#define UART_THR  0
#define UART_DLL  0
#define UART_DLM  1
#define UART_FCR  2
#define UART_LCR  3
#define UART_LSR  5

static void uart_init() {
  volatile uint8_t *u = (volatile uint8_t *)UART_BASE;
  u[UART_LCR] = 0x80;   // DLAB=1, 访问除数寄存器
  u[UART_DLL] = 1;      // 除数低字节 (仿真中随意设一个小值)
  u[UART_DLM] = 0;      // 除数高字节
  u[UART_LCR] = 0x03;   // 8 位数据, 无校验, 1 停止位, DLAB=0
  u[UART_FCR] = 0x07;   // 使能并清空收发 FIFO
}

void putch(char ch) {
  volatile uint8_t *u = (volatile uint8_t *)UART_BASE;
  while ((u[UART_LSR] & 0x20) == 0) ;   // 轮询 THRE (发送保持寄存器空)
  u[UART_THR] = ch;
}

void halt(int code) {
  // ysyxSoC 仿真环境没有"关机"机制; 通过访问 MROM 末尾的 magic 字, 由仿真 TB 的
  // mrom_read 检测到后结束仿真 (等价于讲义里用 ebreak 结束仿真)。
  // code 编码进 magic 地址低 2 位 (字对齐): 0x20000ff0 + ((code&3)<<2)。
  volatile uint32_t *magic = (volatile uint32_t *)(0x20000ff0 + ((code & 3) << 2));
  (void)(*magic);
  while (1);
}

static void puthex(uint32_t v) {
  for (int i = 7; i >= 0; i--) {
    int d = (v >> (i * 4)) & 0xf;
    putch(d < 10 ? '0' + d : 'a' + d - 10);
  }
}

void _trm_init() {
  uart_init();
  // 输出学号标识 CSR: mvendorid="ysyx", marchid=学号
  uint32_t vid, aid;
  asm volatile("csrr %0, 0xf11" : "=r"(vid));
  asm volatile("csrr %0, 0xf12" : "=r"(aid));
  putch('m'); putch('v'); putch('e'); putch('n'); putch('d'); putch('o'); putch('r'); putch('i'); putch('d'); putch('=');
  puthex(vid); putch('\n');
  putch('m'); putch('a'); putch('r'); putch('c'); putch('h'); putch('i'); putch('d'); putch('=');
  puthex(aid); putch('\n');

  int ret = main(mainargs);
  halt(ret);
}
