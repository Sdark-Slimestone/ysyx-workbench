#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
// Verilator仿真必需的头文件
#include "Vexample1.h"       // 对应Verilog模块名top，编译后自动生成
#include "verilated_vcd_c.h"  // VCD波形生成必需的头文件
#include "verilated.h"

int main(int argc, char* argv[]) {
    // 初始化环境
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);//波形追踪启用
    // 实例化Verilog顶层模块
    Vexample1* top = new Vexample1;//有点像malloc
    VerilatedVcdC* wave_trace = new VerilatedVcdC;  //波形相关

    top->trace(wave_trace, 99);                     // 绑定模块到波形跟踪器  void Vexample1::trace(VerilatedVcdC* tfp, int levels); level是层级
    wave_trace->open("obj_dir/example1_wave.vcd");  // 打开波形文件（文件名也能改，比如example114_wave.vcd）
    vluint64_t sim_timestamp = 0;                   // 仿真时间戳（从0开始）

    int test_count = 100;  // 测试次数
    while (test_count--) {
        int a = rand() & 1;
        int b = rand() & 1;

        top->a = a;
        top->b = b;

        top->eval(); //eval计算当前模块的信号输出

        wave_trace->dump(sim_timestamp);  // 将当前时间戳的信号写入波形文件
        sim_timestamp += 1;               // 时间戳+1（模拟1ns步进，数字可改，比如+10=10ns）

        printf("a = %d, b = %d, f = %d\n", a, b, top->f);

        assert(top->f == (a ^ b));
    }

    wave_trace->close();  // 关闭波形文件
    delete wave_trace;    // 释放波形跟踪器内存

    // 补全：释放资源，结束仿真
    delete top;
    printf("\n所有测试通过！共验证100次随机输入\n");
    return 0;
}