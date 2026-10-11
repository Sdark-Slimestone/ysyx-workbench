// 二级 bootloader (FSBL + SSBL)
//  FSBL (本 `entry` 段, 在 flash 中执行): 把 SSBL 从 flash 搬到 SRAM, 跳转到 SRAM 中的 SSBL。
//  SSBL (`.ssbl` 段, 在 SRAM 中执行): 按链接脚本给出的拷贝表 [_copy_table] 逐段把
//        各 loadable 段从 flash(LMA) 搬到 PSRAM(VMA), 再跳转执行。
//  用「逐段拷贝」而非「一段连续拷贝」, 是因为各段的 VMA-LMA 偏移可能因对齐而不同。
.section entry, "ax"
.globl _start
.type _start, @function

_start:
  mv s0, zero
  // ---- FSBL: 搬 SSBL -> SRAM ----
  la t0, _ssbl_lma
  la t1, _ssbl_lma_end
  la t2, _ssbl_start
1:
  bgeu t0, t1, 2f
  lw a0, 0(t0)
  sw a0, 0(t2)
  addi t0, t0, 4
  addi t2, t2, 4
  j 1b
2:
  la t0, _ssbl_start
  jr t0

.size _start, . - _start

// ---------------- SSBL ----------------
.section ssbl, "ax"
.globl _ssbl
.type _ssbl, @function

_ssbl:
  // ---- 按拷贝表逐段搬运 (每项: lma, vma, size; size==0 结束) ----
  // 注意 RV32E 只有 x0-x15, 只能用 t0/t1/t2/a0/a1/s1 等。
  la s1, _copy_table
1:
  lw t0, 0(s1)          // lma
  lw t1, 4(s1)          // vma
  lw t2, 8(s1)          // size
  beqz t0, 9f           // lma==0 -> 表结束
  addi s1, s1, 12
  beqz t2, 1b           // size==0 -> 跳过该段
  add a1, t0, t2        // lma_end
2:
  bgeu t0, a1, 1b
  lw a0, 0(t0)
  sw a0, 0(t1)
  addi t0, t0, 4
  addi t1, t1, 4
  j 2b
9:
  // ---- 清零 .bss ----
  la t0, _bss_start
  la t1, _bss_end
3:
  bgeu t0, t1, 4f
  sw zero, 0(t0)
  addi t0, t0, 4
  j 3b
4:
  la sp, _stack_pointer
  call _trm_init

.size _ssbl, . - _ssbl
