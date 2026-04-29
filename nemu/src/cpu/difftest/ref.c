/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <difftest-def.h>
#include <memory/paddr.h>
#include <string.h>
#include <stdio.h>

extern void init_mem(void);

__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
    uint8_t *host = guest_to_host(addr);
    if (direction == DIFFTEST_TO_REF) {
        memcpy(host, buf, n);
    } else {
        memcpy(buf, host, n);
    }
}

__EXPORT void difftest_regcpy(void *dut, bool direction) {
    uint32_t *reg_buf = (uint32_t *)dut;   // RV32E 所有寄存器均为 32 位
    if (direction == DIFFTEST_TO_REF) {
        // 将 DUT 的状态写入 NEMU
        for (int i = 0; i < 16; i++) {
            cpu.gpr[i] = reg_buf[i];
        }
        printf("=================REF=================\n");
        printf("difftest_regcpy:写入前pc = 0x%08x\n", cpu.pc);
        cpu.pc = reg_buf[16];
        printf("difftest_regcpy:写入后pc = 0x%08x\n", cpu.pc);
    } else {
        // 将 NEMU 的状态读出到 DUT
        for (int i = 0; i < 16; i++) {
            reg_buf[i] = cpu.gpr[i];
        }
        reg_buf[16] = cpu.pc;
    }
}

__EXPORT void difftest_exec(uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        cpu_exec(1);
    }
}

__EXPORT void difftest_raise_intr(word_t NO) {
    // 中断功能暂不支持
    assert(0);
}

__EXPORT void difftest_init(int port) {
    init_mem();
    init_isa();
    // 未使用的寄存器（16..31）清零
    for (int i = RISCV_GPR_NUM; i < 32; i++) {
        cpu.gpr[i] = 0;
    }
    cpu.pc = 0x80000000;
}