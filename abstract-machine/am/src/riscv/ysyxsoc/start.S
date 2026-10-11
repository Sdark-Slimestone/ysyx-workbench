.section entry, "ax"
.globl _start
.type _start, @function

// ysyxSoC: 复位后从 MROM/flash 执行。
// bootloader: 把 .data 从 LMA 搬到 VMA, 再把栈设在 SRAM/PSRAM 末尾。
_start:
  mv s0, zero
#ifdef BOOT_TO_PSRAM
  // 把整个程序 (text+rodata+data) 从 flash (LMA) 搬到 PSRAM (VMA), 再跳转执行
  la t0, _load_start
  la t1, _load_end
  la t2, _run_start
1:
  bgeu t0, t1, 2f
  lw a0, 0(t0)
  sw a0, 0(t2)
  addi t0, t0, 4
  addi t2, t2, 4
  j 1b
2:
  la sp, _stack_pointer
  call _trm_init
#else
  la t0, _data_lma_start     // .data 在 MROM 中的加载地址 (LMA)
  la t1, _data_start         // .data 在 SRAM 中的运行地址 (VMA)
  la t2, _data_end
1:
  bgeu t1, t2, 2f
  lw a0, 0(t0)
  sw a0, 0(t1)
  addi t0, t0, 4
  addi t1, t1, 4
  j 1b
2:
  la sp, _stack_pointer
  call _trm_init
#endif

.size _start, . - _start
