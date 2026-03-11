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

#include <common.h>
#include "monitor/sdb/expr.h"

word_t expr(char *e, bool *success);
void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[]) {
  

  int batch_mode = 0;          // 标记：0不是批处理，1是批处理
  char *test_file = NULL;      // 用来存文件名，一开始没有
  //遍历参数
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) { //如果参数是b或者batch
    batch_mode = 1; // 设为批处理模式
    // 如果 -b 后面还跟着一个参数（且不是以 - 开头的选项）
    if (i + 1 < argc && argv[i+1][0] != '-') {
      test_file = argv[++i];  // 把这个参数当作文件名存起来
    }
    break; // 参数齐全了就停止循环
  }
}

  // 第3步：如果是批处理模式，就执行测试任务
  if (batch_mode) {
    init_regex();
    FILE *fp; // 文件指针
    if (test_file != NULL) {
      fp = fopen(test_file, "r"); // 打开指定的文件
      if (fp == NULL) { // 如果打开失败
        printf("打不开文件\n");
        return 1;
      }
    } else {
      printf("文件呢？\n");
      return 1;
    }

    char line[65536];        // 用来存放读到的每一行文字
    int total = 0, passed = 0; // 统计总数和通过数

    // 一行一行读文件内容，直到读完
    while (fgets(line, sizeof(line), fp)) {
      unsigned expected;     // 存放从文件里读到的预期结果
      char expr_str[65536];  // 存放从文件里读到的表达式
      // 从这一行中解析出 "结果 表达式"，例如 "42 1+2"
      // %u 读无符号整数，%[^\n] 读剩下的所有字符直到换行
      if (sscanf(line, "%u,%[^\n]", &expected, expr_str) != 2) {
        continue; // 如果格式不对，跳过这一行
      }
      total++; // 总行数加1
      bool success; // 用来接收expr函数是否成功
      word_t result = expr(expr_str, &success); // 调用函数计算

      if (success && result == expected) {
        passed++; // 正确就加1
      } else {
        // 错误就打印出来
        printf("FAIL: %s => expected %u, got %u\n", expr_str, expected, (unsigned)result);
      }
    }

    if (fp != stdin) fclose(fp); // 如果打开了文件，就关闭
    printf("Total: %d, Passed: %d, Failed: %d\n", total, passed, total - passed);
    return 0; // 测试完成，直接退出程序，不再进入调试器
  }
  /* Initialize the monitor. */
  #ifdef CONFIG_TARGET_AM
    am_init_monitor();
  #else
    init_monitor(argc, argv);
  #endif

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
