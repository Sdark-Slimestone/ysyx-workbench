# 编译选项配置
CFLAGS=-g -O2 -Wall -Wextra -Isrc -I. -rdynamic -DNDEBUG $(OPTFLAGS)
# Verilator编译选项
VERILATOR_FLAGS = -cc --exe --build --trace -CFLAGS "-DVL_DEBUG $(CFLAGS)"
# 顶层模块名称和要用的测试文件（修改）
TOP_MODULE = example1
TEST_FILE = test1
# 文件路径定义
CPP_FILE = ./csrc/$(TEST_FILE).cpp
VERILOG_FILE = ./vsrc/$(TOP_MODULE).v
# 生成的可执行文件路径
SIM_EXEC = ./obj_dir/V$(TOP_MODULE)
# 波形文件路径
WAVE_FILE = ./obj_dir/$(TOP_MODULE)_wave.vcd

# 默认目标：执行仿真
all: sim

# 仿真目标：编译并运行Verilator仿真
sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Verilator编译中..."	
	verilator $(VERILATOR_FLAGS) $(CPP_FILE) $(VERILOG_FILE) --top-module $(TOP_MODULE)
	@echo "跑模拟中..."
	$(SIM_EXEC)
	@echo "打开波形中..."
	gtkwave $(WAVE_FILE)

# 清理目标：删除编译生成的文件
clean:
	@echo "清理..."
	rm -rf obj_dir
	rm -f $(WAVE_FILE)

# 伪目标声明（避免与同名文件冲突）
.PHONY: all sim clean

-include ../Makefile