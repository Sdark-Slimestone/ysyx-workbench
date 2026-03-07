/* verilator lint_off DECLFILENAME */
module Reg #(WIDTH = 1, RESET_VAL = 0) (
  input clk,
  input rst,
  input [WIDTH-1:0] din,
  output reg [WIDTH-1:0] dout,
  input wen
);
  always @(posedge clk) begin
    if (rst) dout <= RESET_VAL;
    else if (wen) dout <= din;
  end
endmodule

// 选择器模板内部实现
module MuxKeyInternal #(NR_KEY = 2, KEY_LEN = 1, DATA_LEN = 1, HAS_DEFAULT = 0) (
  output reg [DATA_LEN-1:0] out,
  input [KEY_LEN-1:0] key,
  input [DATA_LEN-1:0] default_out,
  input [NR_KEY*(KEY_LEN + DATA_LEN)-1:0] lut
);

  localparam PAIR_LEN = KEY_LEN + DATA_LEN;
  wire [PAIR_LEN-1:0] pair_list [NR_KEY-1:0];
  wire [KEY_LEN-1:0] key_list [NR_KEY-1:0];
  wire [DATA_LEN-1:0] data_list [NR_KEY-1:0];

  genvar n;
  generate
    for (n = 0; n < NR_KEY; n = n + 1) begin
      assign pair_list[n] = lut[PAIR_LEN*(n+1)-1 : PAIR_LEN*n];
      assign data_list[n] = pair_list[n][DATA_LEN-1:0];
      assign key_list[n]  = pair_list[n][PAIR_LEN-1:DATA_LEN];
    end
  endgenerate

  reg [DATA_LEN-1 : 0] lut_out;
  reg hit;
  integer i;
  always @(*) begin
    lut_out = 0;
    hit = 0;
    for (i = 0; i < NR_KEY; i = i + 1) begin
      lut_out = lut_out | ({DATA_LEN{key == key_list[i]}} & data_list[i]);
      hit = hit | (key == key_list[i]);
    end
    if (!HAS_DEFAULT) out = lut_out;
    else out = (hit ? lut_out : default_out);
  end
endmodule

// 带默认值的选择器模板
module MuxKeyWithDefault #(NR_KEY = 2, KEY_LEN = 1, DATA_LEN = 1) (
  output [DATA_LEN-1:0] out,
  input [KEY_LEN-1:0] key,
  input [DATA_LEN-1:0] default_out,
  input [NR_KEY*(KEY_LEN + DATA_LEN)-1:0] lut
);
  MuxKeyInternal #(NR_KEY, KEY_LEN, DATA_LEN, 1) i0 (out, key, default_out, lut);
endmodule



// scpu模块里面的模块
//4bit加法器
module adder4bit (input [3:0] a, input [3:0] b, output [3:0] c);
    assign c = a + b;
endmodule
//带输出选择器的8位寄存器
module reg_with_2output (input clk, input en, input rst, input en_and_in_en, input [7:0]in, input out1en, input out2en, output [7:0]out1, output [7:0]out2, output[7:0]direct);
    wire [7:0]reg2mux;
    wire en_and_gate;
    wire [7:0]in_and_gate;
    Reg #(8, 8'b00000000) grfreg (clk, rst, in_and_gate, reg2mux, en_and_gate);
    assign out1 = {8{out1en}} & reg2mux;
    assign out2 = {8{out2en}} & reg2mux;
    assign en_and_gate = en_and_in_en & en;
    assign in_and_gate = {8{en_and_in_en}} & in;
    assign direct = reg2mux;
endmodule

//scpu大模块
//pc
module pc (input clk, input en, input jalmux, input rst, input [3:0]jal2mux, output [3:0] pc2romaddr);
    wire [3:0] a;
    wire [3:0] regout;
    wire [3:0] mux2reg;
    wire [3:0] add2mux;

    assign a = 4'b0001;    
    assign mux2reg = (jalmux == 1)? jal2mux : add2mux;

    adder4bit pcadd (a, regout, add2mux);
    Reg #(4, 4'b0000) pc_reg (clk, rst, mux2reg, regout, en);

    assign pc2romaddr = regout;
endmodule

//8位加法器
module adder (input [7:0] a, input [7:0] b, output [7:0] out );
    assign out = a + b;
endmodule

//GRF,en接对应的,rst接一个恒为0的常量
/* verilator lint_off UNUSEDSIGNAL */
module grf (input clk, input rden, input rst, 
            input out1en, input out2en,
            input imm_or_not, input [3:0]imm, //imm_or_not接译码器的immen
            input [1:0] out1addr, input [1:0] out2addr, input [1:0] inaddr, 
            input [7:0] in, output [7:0] out1, output [7:0]out2, output[7:0]r0, output[7:0]r2);

    //判断是否是imm
    wire [7:0] reg_in;
    wire [7:0] imm_premux;
    wire [1:0] choose;
    assign imm_premux ={4'b0000, imm};
    MuxKeyWithDefault #(2,1,2) imm_judge (choose, imm_or_not,2'b00,{1'b0,2'b01,1'b1,2'b10});
    assign reg_in = ({8{choose[0]}} & in) | ({8{choose[1]}} & imm_premux);
   
    // input译码器
    wire [3:0] in_mux2reg_en_andgate;
    //wire [3:0] reg_en_andgate2reg_en;
    MuxKeyWithDefault #(4,2,4) in_addr (in_mux2reg_en_andgate, inaddr,4'b0000,{2'b00,4'b0001,2'b01,4'b0010,2'b10,4'b0100,2'b11,4'b1000});
    // assign in_mux2reg_en_and_ = inaddren & inmux_out; //译码器输出有en，但实际不需要，只需要译码器输出直接接到寄存器本身en的与门上，然后与门另一条线接rden?
    //assign reg_en_andgate2reg_en = in_mux2reg_en_andgate & rden;


    // out1译码器
    wire [3:0] out1_mux2reg_out1en;
    wire [3:0] out1mux_out;
    MuxKeyWithDefault #(4,2,4) out1_addr (out1mux_out, out1addr,4'b0000,{2'b00,4'b0001,2'b01,4'b0010,2'b10,4'b0100,2'b11,4'b1000});
    assign out1_mux2reg_out1en = out1mux_out & {4{out1en}};
    // out2译码器
    wire [3:0] out2_mux2reg_out2en;
    wire [3:0] out2mux_out;
    MuxKeyWithDefault #(4,2,4) out2_addr (out2mux_out, out2addr,4'b0000,{2'b00,4'b0001,2'b01,4'b0010,2'b10,4'b0100,2'b11,4'b1000});
    assign out2_mux2reg_out2en = out2mux_out & {4{out2en}};

    //寄存器堆
    wire [7:0] out1_0, out1_1, out1_2, out1_3;
    wire [7:0] out2_0, out2_1, out2_2, out2_3;
    wire [7:0] r1, r3;
    reg_with_2output grf0 (clk, rden, rst, in_mux2reg_en_andgate[0], reg_in, out1_mux2reg_out1en[0], out2_mux2reg_out2en[0], out1_0, out2_0, r0);
    reg_with_2output grf1 (clk, rden, rst, in_mux2reg_en_andgate[1], reg_in, out1_mux2reg_out1en[1], out2_mux2reg_out2en[1], out1_1, out2_1, r1);
    reg_with_2output grf2 (clk, rden, rst, in_mux2reg_en_andgate[2], reg_in, out1_mux2reg_out1en[2], out2_mux2reg_out2en[2], out1_2, out2_2, r2);
    reg_with_2output grf3 (clk, rden, rst, in_mux2reg_en_andgate[3], reg_in, out1_mux2reg_out1en[3], out2_mux2reg_out2en[3], out1_3, out2_3, r3);
    assign out1 = out1_0 | out1_1 | out1_2 | out1_3;
    assign out2 = out2_0 | out2_1 | out2_2 | out2_3;
endmodule
/* verilator lint_off UNUSEDSIGNAL */

//数值比较器
module comparator (input en, input [7:0] r0, input [7:0] rs2, output out2jalmux); //en接addren

    wire [7:0] xor_result; 
    assign xor_result = r0 ^ rs2;

    wire result;
    assign result = |xor_result;  

    assign out2jalmux = result & en;
endmodule
//译码器
/* verilator lint_off UNUSEDSIGNAL */
module idu (input [7:0]instruction, 
            output rden, output rs1en, output rs2en, output addren, output immen,
            output [1:0]rd, output [1:0]rs1, output [1:0]rs2,
            output [3:0]addr, output [3:0]imm);
    wire add, li, bner0;
    wire [3:0] opdecode;
    MuxKeyWithDefault #(4,2,4) op_decoder (opdecode, instruction[7:6],4'b0000,{2'b00,4'b0001,2'b01,4'b0010,2'b10,4'b0100,2'b11,4'b1000});
    assign add = opdecode[0]; //00 rd = rs1 + rs2
    //assign out = opdecode[1]; //01 暂时不管
    assign li = opdecode[2]; //10 rd = imm
    assign bner0 = opdecode[3]; //11 if（r0 ！= rs2) PC = addr
    assign rden = add | li;
    assign rs1en = add;
    assign rs2en = add | bner0;
    assign addren = bner0;
    assign immen = li;
    assign rd =  instruction[5:4];
    assign rs1 = instruction[3:2];
    assign rs2 = instruction[1:0];
    assign addr = instruction[5:2];
    assign imm = instruction [3:0];
endmodule
/* verilator lint_off UNUSEDSIGNAL */

module rom(input[3:0]romaddr, output[7:0]instruction);
    MuxKeyWithDefault #(16, 4, 8) rom_decoder (
        .out(instruction),
        .key(romaddr),
        .default_out(8'h00),
        .lut({
            4'h0, 8'b10001010,   // 地址0 数据0
            4'h1, 8'b10010000,   // 地址1 数据1
            4'h2, 8'b10100000,
            4'h3, 8'b10110001,
            4'h4, 8'b00010111,
            4'h5, 8'b00101001,
            4'h6, 8'b11010001,
            4'h7, 8'b11011111,
            4'h8, 8'h00,
            4'h9, 8'h00,
            4'ha, 8'h00,
            4'hb, 8'h00,
            4'hc, 8'h00,
            4'hd, 8'h00,
            4'he, 8'h00,
            4'hf, 8'h00    // 地址15 数据15
        })
    );
endmodule

module scpu (
    input clk,
    input rst,
    output [7:0] r2_out
);

    wire [3:0] pc2romaddr;
    wire [7:0] instruction;
    wire rden, rs1en, rs2en, addren, immen;
    wire [1:0] rd, rs1, rs2;
    wire [3:0] addr, imm;
    wire [7:0] out1, out2, r0, r2;
    wire [7:0] alu_result;
    wire jalmux;

    pc pc_inst (clk, 1'b1, jalmux, rst, addr, pc2romaddr);
    rom rom_inst (pc2romaddr, instruction);
    idu idu_inst (instruction, rden, rs1en, rs2en, addren, immen, rd, rs1, rs2, addr, imm);
    grf grf_inst (clk, rden, rst, rs1en, rs2en, immen, imm, rs1, rs2, rd, alu_result, out1, out2, r0, r2);
    adder adder_inst (out1, out2, alu_result);
    comparator comp_inst (addren, r0, out2, jalmux);

    assign r2_out = r2;

endmodule


