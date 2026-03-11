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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// 缓冲区大小，用于存放生成的表达式
static char buf[65536] = {};
// 用于存放完整的C程序代码（比buf稍大，以便容纳模板）
static char code_buf[65536 + 128] = {};
// C程序的模板，其中 %s 会被替换为生成的表达式
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

//辅助函数：生成随机无符号数
static void gen_num() {
  // 生成一个32位的随机无符号整数
  unsigned val = ((unsigned)rand() << 16) ^ (unsigned)rand(); //当作max是32000多少来着
  char tmp[20];                        // 足够存放10位数字 + 'u' + '\0'
  sprintf(tmp, "%u", val);              // 将数值转为十进制字符串
  int len = strlen(tmp);
  tmp[len] = 'u';                       // 添加 'u' 后缀，强制为无符号常量
  tmp[len + 1] = '\0';                   
  strcat(buf, tmp);                     // 追加到全局缓冲区
}

//辅助函数：随机生成运算符
static char gen_rand_op() {
  int op = rand() % 4;                   // 0,1,2,3
  switch (op) {
    case 0: return '+';
    case 1: return '-';
    case 2: return '*';
    default: return '/';
  }
}

//辅助函数：向缓冲区添加单个字符
static void gen(char c) {
  char str[2] = {c, '\0'};
  strcat(buf, str);
}

//辅助函数：随机插入空格
static void gen_space() {
  // 以30%的概率插入空格
  if (rand() % 10 < 3) {
    int n = rand() % 3 + 1;              // 随机1~3个空格
    for (int i = 0; i < n; i++) {
      strcat(buf, " ");
    }
  }
}

//核心递归生成函数
// depth 控制递归深度，防止无限递归和缓冲区溢出
static void gen_expr(int depth) {
  // 如果深度超过8，强制生成数字（递归终止条件）
  if (depth > 6) {
    gen_num();
    return;
  }

  int choice = rand() % 4;               // 随机选择四种生成方式之一
  switch (choice) {
    case 0:  // 直接生成一个数字
      gen_space();   // 可选的空格
      gen_num();
      gen_space();
      break;

    case 1:  // 生成括号表达式: ( expr )
      gen_space();
      gen('(');
      gen_space();
      gen_expr(depth + 1);   // 递归生成括号内的表达式，深度+1
      gen_space();
      gen(')');
      gen_space();
      break;
    case 2: //生成一元表达式 -
      gen_space();
      if(buf[0] != '\0' && buf[strlen(buf) - 1] == '-'){
        strcat(buf, " ");
      }
      gen('-');
      gen('(');
      gen_space();
      gen_expr(depth + 1);
      gen_space();
      gen(')');
      break;
    default: // 生成二元运算: expr op expr
      gen_expr(depth + 1);   // 左操作数
      gen_space();
      char op = gen_rand_op();
      // 避免出现连续两个减号--，看起来似乎多余，但留着也没坏处吧（（（
      if (buf[0] != '\0' && buf[strlen(buf)-1] == '-' && op == '-') {
        strcat(buf, " ");    // 插入一个空格分隔
      }
      gen(op);               // 添加运算符
      gen_space();
      gen_expr(depth + 1);   // 右操作数
      break;
  }
}

//对外接口：生成随机表达式
static void gen_rand_expr() {
  buf[0] = '\0';        // 清空缓冲区
  gen_expr(0);          // 从深度0开始递归
}

//主函数
int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);   // 从命令行获取要生成的表达式数量
  }

  for (int i = 0; i < loop; i++) {
    gen_rand_expr();                 // 生成随机表达式到 buf

    // 将表达式填入模板，得到完整的C程序
    sprintf(code_buf, code_format, buf);

    // 将C程序写入临时文件
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // 编译临时文件，捕获编译器输出，检查除零警告
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "gcc /tmp/.code.c -o /tmp/.expr 2>&1");
    FILE *compile_fp = popen(cmd, "r");
    if (compile_fp == NULL) {
      continue;  // popen失败也跳过
    }
    char output[1024] = {0};
    size_t n = fread(output, 1, sizeof(output)-1, compile_fp);
    (void)n;                                                            //忽略
    int ret = pclose(compile_fp);
    if (ret != 0 || strstr(output, "division by zero") != NULL) {
      continue;  // 编译失败或出现除零警告，跳过该表达式
    }

    // 运行可执行文件，并读取其输出
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    unsigned result;
    int scanned = fscanf(fp, "%u", &result);
    int status = pclose(fp);
    if (scanned != 1 || status != 0) {
    // 运行失败（可能除零或其他错误），跳过该表达式
    continue;
    }
    printf("%u,%s\n", result, buf);
  }
  return 0;
}