cmd_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/monitor/monitor.o := unused

source_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/monitor/monitor.o := src/monitor/monitor.c

deps_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/monitor/monitor.o := \
    $(wildcard include/config/trace.h) \
    $(wildcard include/config/target/am.h) \
    $(wildcard include/config/device.h) \
    $(wildcard include/config/itrace.h) \
  /home/sdark/ysyx-workbench/nemu/include/isa.h \
  /home/sdark/ysyx-workbench/nemu/src/isa/riscv32/include/isa-def.h \
    $(wildcard include/config/rve.h) \
    $(wildcard include/config/rv64.h) \
  /home/sdark/ysyx-workbench/nemu/include/common.h \
    $(wildcard include/config/mbase.h) \
    $(wildcard include/config/msize.h) \
    $(wildcard include/config/isa64.h) \
  /home/sdark/ysyx-workbench/nemu/include/macro.h \
  /home/sdark/ysyx-workbench/nemu/include/debug.h \
  /home/sdark/ysyx-workbench/nemu/include/utils.h \
    $(wildcard include/config/target/native/elf.h) \
  /home/sdark/ysyx-workbench/nemu/include/memory/paddr.h \
    $(wildcard include/config/pc/reset/offset.h) \
  src/monitor/../utils/ftrace.h \

/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/monitor/monitor.o: $(deps_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/monitor/monitor.o)

$(deps_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/monitor/monitor.o):
