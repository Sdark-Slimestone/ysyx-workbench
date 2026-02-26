#include "lcthw/architecture.h"
#include "lcthw/isa.h"
#include "lcthw/dbg.h"
#include <stdio.h>
#include <stdlib.h>
#include <am.h>
#include <klib-macros.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    
    const char *rom_file = "/home/sdark/ysyx-workbench/minirv3/bin/vga.bin";
    const char *ram_file = "/home/sdark/ysyx-workbench/minirv3/bin/vga.bin";

    // 不再检查argc，直接使用固定路径
    if (init_rom(rom_file) != 0) {
        fprintf(stderr, "错误：无法初始化 ROM (文件: %s)\n", rom_file);
        return 1;
    }
    if (init_ram(ram_file) != 0) {
        fprintf(stderr, "错误：无法初始化 RAM (文件: %s)\n", ram_file);
        free(ROM);
        return 1;
    }

    GFR[PC] = 0;
    int ret = 0;

    while (1) {
        int pc = GFR[PC];
        int instr = read_rom(pc / 4);
        if (instr == -1) {
            fprintf(stderr, "错误：无法读取 PC=0x%x 处的指令\n", pc);
            break;
        }
        ret = decode(instr);
        if(ret == 1){ //只有r10 = 0 也就是 退出码= 0+1时退出，其他情况不处理
            break;
        }
    }
    //显示↓
    printf("模拟结束，准备显示...\n");
    
    ioe_init();  // 初始化 AM 的 I/O 设备（包括图形）
    printf("ioe_init 完成\n");

    // 将 VRAM 绘制到屏幕 (0,0) 位置，并立即刷新
    io_write(AM_GPU_FBDRAW, 0, 0, VRAM, 256, 256, true);
    printf("绘制完成\n");

    //等待按键后退出
    while (1) {
        AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
        if (ev.keydown) break; // 按下任意键退出
    }
    //显示↑

    free(RAM);
    free(ROM);
    return 0;
}


//小抄↓
//make ARCH=native
//./build/minirv-native.elf