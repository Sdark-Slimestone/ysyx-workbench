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
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>   //内存操作接口
#include "sdb.h"
#include "watchpoint.h"
#include "expr.h"
#include <isa.h>


static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_info(char *args) {
  char arg = '\0';
  if(args == NULL){
    printf("参数呢？？？？\n");
    return 0;
  }else{
    if(sscanf(args, "%c", &arg) != 1){
      printf("参数格式错误，请输入 r 或者 w\n");
      return 0;
    }else{                   //参数格式正确，是个char，但不知道是啥
      if(arg == 'r'){
        isa_reg_display();
        return 0;
      } else if(arg == 'w'){
        info_watchpoints(); 
        return 0;
      } else{
        printf("未知命令，请输入r或者w\n");
        return 0;
      }
    }
  }
}

static int cmd_w(char *args) {
    if (args == NULL) {
      printf("请指定要监视的表达式\n");
      return 0;
    }
    WP *wp = new_wp(args);  // 内部已处理表达式求值和资源分配，失败时
    if (wp == NULL){
      printf("设置节点失败\n");
      return 0;
    }
    printf("已设置监视点 %d: %s\n", wp->NO, wp->exp);
    return 0;
}


static int cmd_d(char *args) {
    char *arg = strtok(NULL, " ");
    if (arg == NULL) {
      printf("缺少监视点编号\n");
      return 0;
    }
    int no = atoi(arg);
    delete_watchpoint(no);
    return 0;
}

static int cmd_si(char *args) {
  uint64_t n = 1; // 默认执行1条指令（单步）
  if(args != NULL){
    if(sscanf(args, "%lu", &n) != 1){
      printf("参数解析失败，格式为%%lu\n");
      n = 1;
      cpu_exec(n);
      return 0;
    }else{
      printf("参数解析成功，n = %lu\n,如n=0，则什么都不做捏\n", n);
      cpu_exec(n);
      return 0;
    }
  }else{
    printf("参数为空，默认跑一周期\n");
    cpu_exec(n);
    return 0;
  }
}

static int cmd_x(char *args) {
  int n;          
  uint32_t addr;  

  if (sscanf(args, "%d %x", &n, &addr) != 2) {
    printf("命令格式错误，正确格式：x 个数 十六进制地址（例：x 10 0x80000000）\n");
    return 0;
  }

  if (n <= 0) {
    printf("个数必须是正整数！\n");
    return 0;
  }

  printf("内存扫描：起始地址 0x%x，共%d个字(个4字节)\n", addr, n);
  printf("地址          数值（16进制）\n");
  printf("-----------------------------\n");
  for (int i = 0; i < n; i++) {
    word_t data = paddr_read(addr + i*4, 4);    //第一个参数是起始地址，第二个参数是字节数
    printf("0x%08x    0x%08x\n", addr + i*4, data);
  }

  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("请指定要求值的表达式\n");
    return 0;
  }
  bool success;
  word_t result = expr(args, &success);
  if (success) {
    printf("结果为: %u (0x%x)\n", result, result);
  } else {
    printf("表达式求值失败\n");
  }
  return 0;
}


static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);   //所有命令处理函数都是返回int，参数是字符串
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },   //cmd命令表
  { "c", "Continue the execution of the program", cmd_c },
  { "si", "Step execute %%lu instructions (single step)", cmd_si }, // 新增si命令
  { "info", "打印信息，r是寄存器值，w是监视点信息", cmd_info },  //新增info命令
  { "x", "扫描内存信息", cmd_x },  //新增info命令
  { "q", "Exit NEMU", cmd_q },
  { "d", "删除对应的监视点 d N", cmd_d },
  { "w", "设置监视点 ", cmd_w },
  { "p", "表达式求值 exp", cmd_p },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {                            //sdb主循环
  if (is_batch_mode) {                            //batch模式就一直跑。跑完退出不叫胡
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {      //交互模式
    char *str_end = str + strlen(str);                //标记字符串末尾

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");                     //把输入的字符串按空格拆分，取第一个词当“命令”
    if (cmd == NULL) { continue; }                    // 如果只按了回车（没输入任何东西），重新等输入

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;         // 命令后面的部分当“参数”
    if (args >= str_end) {                       // 如果只有命令没有参数
      args = NULL;                                  // 把参数设为NULL，避免后续出错
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;                                            //这里在遍历命令表，找到对应命令就执行，然后退出循环
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }   //没找到命令
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
