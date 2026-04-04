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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>   
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sdb.h"
#include "watchpoint.h"
#include "expr.h"
#include <assert.h>


enum {
  TK_NOTYPE = 256,  // 
  TK_EQ,            // == 双等号 257
  TK_DEC,           // 十进制整数 258
  TK_ADD,           // 加号 + 259
  TK_SUB,           // 减号 - 260
  TK_NEG,           // 负号 -
  TK_MUL,           // 乘号 * 
  TK_DIV,           // 除号 / 
  TK_LPAREN,        // 左括号 ( 
  TK_RPAREN,        // 右括号 ) 
  TK_DEREF,         // 解引用 *
  TK_NEQ,           // !=
  TK_AND,           // &&
  TK_HEX,           // 十六进制数 (以0x开头)
  TK_REG            // 寄存器 (以$开头)
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  // 多字符运算符和特殊 token 优先匹配
  {"\\(unsigned\\)", TK_NOTYPE},      // 忽略类型转换标记  <-- 新增
  {"0[xX][0-9a-fA-F]+u", TK_HEX},      // 带u后缀的十六进制数
  {"0[xX][0-9a-fA-F]+", TK_HEX},      // 十六进制数
  {"[0-9]+u", TK_DEC},                 // 带u后缀的十进制数
  {"[0-9]+", TK_DEC},                   // 普通十进制数
  {"\\$[a-zA-Z0-9_]+", TK_REG},        // 寄存器，如 $eax
  {"==", TK_EQ},                        // 等于
  {"!=", TK_NEQ},                       // 不等于
  {"&&", TK_AND},                       // 逻辑与
  {"\\+", TK_ADD},                       // 加号
  {"\\-", TK_SUB},                       // 减号
  {"\\*", TK_MUL},                       // 乘号（也可能是解引用，后面处理）
  {"\\/", TK_DIV},                       // 除号
  {"\\(", TK_LPAREN},                     // 左括号
  {"\\)", TK_RPAREN},                     // 右括号
  {" +", TK_NOTYPE},                      // 空格（忽略）
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);    //第一个参数是存编译后规则的结构体的指针，第二个是包含正则表达式的字符串，第三个是标志，这里用的是使用扩展正则表达式
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token { //token大小
  int type;
  char str[128];
} Token;

static Token tokens[1024] __attribute__((used)) = {};   // 改为 1024   //存字符串
static int nr_token __attribute__((used))  = 0;
int stage = 0;

static bool make_token(char *e) {                                           
  int position = 0;                                                            
  int i;                                                                         
  regmatch_t pmatch;                                                                  
  nr_token = 0;                                                                           
  while (e[position] != '\0') {                                                               
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {                                                             
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {             

        char *substr_start = e + position;           //匹配成功的子串（substr）的起始是e+position
        int substr_len = pmatch.rm_eo;               //匹配到的子串的长度是 eo-so 

        position += substr_len;  //然后把position偏移量往后挪 之前匹配到的字符串的长度/

        /*存入tokens数组的功能部分*/
        if (rules[i].token_type != TK_NOTYPE) {                                                                   //不是空格就存进tokens
          int copy_len = substr_len;
          if(nr_token >= 1023){ printf("token数组越界，保证输入小于1023个字符\n"); return false;}
          
          //数字去掉末尾的 'u'，寄存器去掉开头的 '$'
          if (rules[i].token_type == TK_DEC && copy_len > 0 && substr_start[copy_len-1] == 'u') {
            copy_len--;   // 去掉 u
          }else if (rules[i].token_type == TK_HEX && copy_len > 0 && substr_start[copy_len-1] == 'u') {
            copy_len--;   // 去掉十六进制的 u
          }else if (rules[i].token_type == TK_REG && copy_len > 0 && substr_start[0] == '$') {
            substr_start++;  // 跳过 $
            copy_len--;
          }
          // 其他类型（包括十六进制）直接复制

          if(copy_len >= 127){ printf("单个字符过长，保证单字符串小于128字符\n"); return false;}

          strncpy(tokens[nr_token].str, substr_start, copy_len);                 
          tokens[nr_token].str[copy_len] = '\0';
          tokens[nr_token].type = rules[i].token_type;

          nr_token++; // 计数+1
        }

        break;
      }
    }

    if (i == NR_REGEX) {                                                                      //所有规则匹配失败
      printf("匹配不到符号\n");
      return false;
    }
  }
  /* 重新遍历一遍tokens，区分负号和减号，上面是统一识别为减号的，同时区分乘号和解引用 */
  for (int j = 0; j < nr_token; j++) {
    if (tokens[j].type == TK_SUB) { //如果发现减号就开始判断
      if ((j == 0) ||                       //是第一个token
          (tokens[j-1].type == TK_ADD || tokens[j-1].type == TK_SUB || 
          tokens[j-1].type == TK_MUL || tokens[j-1].type == TK_DIV ||
          tokens[j-1].type == TK_EQ || tokens[j-1].type == TK_NEQ ||
          tokens[j-1].type == TK_AND) || 
          (tokens[j-1].type == TK_LPAREN) || //前一位是左括号
          (tokens[j-1].type == TK_NEG)) {  //前一位是负号
        tokens[j].type = TK_NEG;  // 把减号替换为负号
      }
    }
    else if (tokens[j].type == TK_MUL) { // 乘号，可能是一元解引用
      if (j == 0 ||
          tokens[j-1].type == TK_ADD || tokens[j-1].type == TK_SUB ||
          tokens[j-1].type == TK_MUL || tokens[j-1].type == TK_DIV ||
          tokens[j-1].type == TK_EQ || tokens[j-1].type == TK_NEQ ||
          tokens[j-1].type == TK_AND || 
          tokens[j-1].type == TK_LPAREN ||
          tokens[j-1].type == TK_NEG || tokens[j-1].type == TK_DEREF) {
        tokens[j].type = TK_DEREF;   // 改为解引用
      }
    }
  }
  return true;
}

static word_t eval(int l, int r, bool *success) {
  if (l > r) {
    *success = false;
    return 0;
  }

  // 单个 token
  if (l == r) {
    Token *t = &tokens[l];
    switch (t->type) {
      case TK_DEC: {
        char *endptr;
        word_t val = strtoul(t->str, &endptr, 10);
        if (*endptr != '\0') {
          *success = false;
          return 0;
        }
        return val;
      }
      case TK_HEX: {
        char *endptr;
        word_t val = strtoul(t->str, &endptr, 16);
        if (*endptr != '\0') {
          *success = false;
          return 0;
        }
        return val;
      }
      case TK_REG: {
        bool reg_success = true;
        word_t val = isa_reg_str2val(t->str, &reg_success);
        if (!reg_success) {
          *success = false;
          return 0;
        }
        return val;
      }
      default:
        *success = false;
        return 0;
    }
  }

  // 检查是否被一对括号完全包裹
  if (tokens[l].type == TK_LPAREN && tokens[r].type == TK_RPAREN) {
    int level = 0;
    int i;
    for (i = l; i <= r; i++) {
      if (tokens[i].type == TK_LPAREN) level++;
      else if (tokens[i].type == TK_RPAREN) level--;
      if (level == 0 && i < r) break; // 提前闭合，不是完全包裹
      if (i == r && level == 0) {
        return eval(l + 1, r - 1, success); // 去掉外层括号
      }
    }
  }

  // 查找不在括号内的最低优先级二元运算符
  int op_pos = -1;
  int op_type = -1;

  int prio_groups[][4] = {
    {TK_AND, 0},
    {TK_EQ, TK_NEQ, 0},
    {TK_ADD, TK_SUB, 0},
    {TK_MUL, TK_DIV, 0},
  };
  int group_count = sizeof(prio_groups) / sizeof(prio_groups[0]);

  for (int g = 0; g < group_count; g++) {
    int level = 0;
    int last_op = -1;
    //再tokens中找括号外的算符
    for (int i = l; i <= r; i++) {
      if (tokens[i].type == TK_LPAREN) {
        level++;
      } else if (tokens[i].type == TK_RPAREN) {
        level--;
      } else if (level == 0) {
        //找当前再tokens中找到的算符属于哪个优先级组，找到的话就返回算符的位置，找不到，那肯定就是单目
        for (int k = 0; prio_groups[g][k] != 0; k++) {
          if (tokens[i].type == prio_groups[g][k]) {
            last_op = i;
            break;
          }
        }
      }
    }
    if (last_op != -1) {
      op_pos = last_op;
      op_type = tokens[last_op].type;
      break;
    }
  }

  if (op_pos != -1) {
    word_t left_val = eval(l, op_pos - 1, success);  //先算左值，再算右值 再算左右运算后的值
    if (!*success) return 0;

    switch (op_type) {
      case TK_ADD: {
        word_t right_val = eval(op_pos + 1, r, success);
        if (!*success) return 0;
        word_t result = left_val + right_val;
        return result;
      }
      case TK_SUB: {
        word_t right_val = eval(op_pos + 1, r, success);
        if (!*success) return 0;
        word_t result = left_val - right_val;
        return result;
      }
      case TK_MUL: {
        word_t right_val = eval(op_pos + 1, r, success);
        if (!*success) return 0;
        word_t result = left_val * right_val;
        return result;
      }
      case TK_DIV: {
        word_t right_val = eval(op_pos + 1, r, success);
        if (!*success) return 0;
        if (right_val == 0) {
          printf("除法错误：除数为零\n");
          *success = false;
          return 0;
        }
        word_t result = left_val / right_val;
        return result;
      }
      case TK_EQ: {
        word_t right_val = eval(op_pos + 1, r, success);
        if (!*success) return 0;
        word_t result = (left_val == right_val);
        return result;
      }
      case TK_NEQ: {
        word_t right_val = eval(op_pos + 1, r, success);
        if (!*success) return 0;
        word_t result = (left_val != right_val);
        return result;
      }
      case TK_AND: {
        if (left_val == 0) {
          return 0;
        }
        word_t right_val = eval(op_pos + 1, r, success);
        if (!*success) return 0;
        word_t result = (right_val != 0) ? 1 : 0;
        return result;
      }
      default:
        *success = false;
        return 0;
    }
  }

  // 处理一元运算符（负号、解引用）
  if (tokens[l].type == TK_NEG || tokens[l].type == TK_DEREF) {
    word_t sub_val = eval(l + 1, r, success);
    if (!*success) return 0;
    if (tokens[l].type == TK_NEG) {
      word_t result = 0 - sub_val;
      return result;
    } else { // TK_DEREF
      if((sub_val % 4) != 0){
        printf("非法解引用, 但也不是不能解（\n");
      }
      word_t result = paddr_read(sub_val, 4);
      return result;
    }
  }

  *success = false;
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  *success = true;
  return eval(0, nr_token - 1, success);
}