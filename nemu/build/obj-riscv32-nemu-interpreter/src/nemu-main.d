cmd_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o := unused

source_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o := src/nemu-main.c

deps_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o := \
    $(wildcard include/config/target/am.h) \
  /home/sdark/ysyx-workbench/nemu/include/common.h \
    $(wildcard include/config/mbase.h) \
    $(wildcard include/config/msize.h) \
    $(wildcard include/config/isa64.h) \
  /home/sdark/ysyx-workbench/nemu/include/macro.h \
  /home/sdark/ysyx-workbench/nemu/include/debug.h \
  /home/sdark/ysyx-workbench/nemu/include/utils.h \
    $(wildcard include/config/target/native/elf.h) \
  src/monitor/sdb/expr.h \
  /home/sdark/ysyx-workbench/nemu/include/cpu/cpu.h \
  /home/sdark/ysyx-workbench/nemu/include/memory/paddr.h \
    $(wildcard include/config/pc/reset/offset.h) \
  src/monitor/sdb/sdb.h \
  src/monitor/sdb/watchpoint.h \
  src/monitor/sdb/expr.h \

/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o: $(deps_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o)

$(deps_/home/sdark/ysyx-workbench/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o):
