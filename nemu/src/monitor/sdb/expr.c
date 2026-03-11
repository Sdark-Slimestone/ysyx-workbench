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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "memory/paddr.h" 

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
  {"0[xX][0-9a-fA-F]+", TK_HEX},      // 十六进制数
  {"[0-9]+u", TK_DEC},                 // 带u后缀的十进制数
  {"[0-9]+", TK_DEC},                   // 普通十进制数
  {"\\$[a-zA-Z0-9_]+", TK_REG},        // 寄存器，如 $eax
  {"==", TK_EQ},                        // 等于
  {"!=", TK_NEQ},                       // 不等于
  {"&&", TK_AND},                       // 逻辑与
  {"\\+", TK_ADD},                       // 加号
  {"\\-", TK_SUB},                       // 减号
  {"\\*", TK_MUL},                       // 乘号（也可能是解引用，后处理）
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

static Token tokens[1024] __attribute__((used)) = {};   // 改为 1024
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {                                                //定义一个静态布尔型函数make_token，参数e是要处理的字符串，函数返回真假值（成功 / 失败）。
  int position = 0;                                                               //定义变量position并设为 0，用来记当前处理字符串到哪个位置了。
  int i;                                                                           //定义整型变量i，用来循环遍历规则。
  regmatch_t pmatch;                                                                  //定义pmatch变量，专门存正则匹配出来的位置信息。
  nr_token = 0;                                                                           //token数量计数
  while (e[position] != '\0') {                                                               //只要当前位置的字符不是字符串结束符，就一直循环处理。
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {                                                             //循环遍历所有正则规则（NR_REGEX是规则总数，最上面的一个宏），从第 0 个规则开始试。
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {             

        char *substr_start = e + position;           //匹配成功的子串（substr）的起始是e+position
        int substr_len = pmatch.rm_eo;               //匹配到的子串的长度是 eo-so = eo-0 = eo

        /*Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start); */

        position += substr_len;  //然后把position偏移量往后挪 之前匹配到的字符串的长度/

        /*存入tokens数组的功能部分*/
        if (rules[i].token_type != TK_NOTYPE) {                                                                   //不是空格就存进tokens
          int copy_len = substr_len;
          if(nr_token >= 1023){ printf("token数组越界，保证输入小于1023个字符\n"); return false;}
          
          // 特殊处理：数字去掉末尾的 'u'，寄存器去掉开头的 '$'
          if (rules[i].token_type == TK_DEC && copy_len > 0 && substr_start[copy_len-1] == 'u') {
            copy_len--;   // 去掉 u
          } else if (rules[i].token_type == TK_REG && copy_len > 0 && substr_start[0] == '$') {
            substr_start++;  // 跳过 $
            copy_len--;
          }
          // 其他类型（包括十六进制）直接复制

          if(copy_len >= 127){ printf("单个字符过长，保证单字符串小于128字符\n"); return false;}

          strncpy(tokens[nr_token].str, substr_start, copy_len);                                                  // 目标数组 要复制的字符串起点，复制的数量
          tokens[nr_token].str[copy_len] = '\0';
          tokens[nr_token].type = rules[i].token_type;

          nr_token++; // 计数+1
          }

        break;
      }
    }

    if (i == NR_REGEX) {                                                                      //所有规则匹配失败
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  /*重新遍历一遍tokens，区分负号和减号，上面是统一识别为减号的，同时区分乘号和解引用*/
  for (int j = 0; j < nr_token; j++) {
    if (tokens[j].type == TK_SUB) { //如果发现减号就开始判断
      if ((j == 0) ||                       //是第一个token
          (tokens[j-1].type == TK_ADD || tokens[j-1].type == TK_SUB || 
           tokens[j-1].type == TK_MUL || tokens[j-1].type == TK_DIV) || //前一位是运算符 
          (tokens[j-1].type == TK_LPAREN) || //前一位是左括号
          (tokens[j-1].type == TK_NEG)) {  //前一位是负号
        tokens[j].type = TK_NEG;  // 把减号替换为负号
      }
    }
    else if (tokens[j].type == TK_MUL) { // 乘号，可能是一元解引用
        if (j == 0 ||
            tokens[j-1].type == TK_ADD || tokens[j-1].type == TK_SUB ||
            tokens[j-1].type == TK_MUL || tokens[j-1].type == TK_DIV ||
            tokens[j-1].type == TK_LPAREN ||
            tokens[j-1].type == TK_NEG || tokens[j-1].type == TK_DEREF) {
          tokens[j].type = TK_DEREF;   // 改为解引用
      }
    }
  }
  return true;
}

//求值部分

//运算+防除以零情况功能函数
// 修改：直接使用 token_type 而非 char，支持更多运算符
uint32_t apply_op(uint32_t left, int op_type, uint32_t right, bool *ok, bool is_unary) {
  if(is_unary == true){
    switch(op_type){
      case TK_NEG: return 0 - right;                                   // 一元负号
      case TK_DEREF:                                                   // 一元解引用：读取内存
        // 调用内存读取函数 vaddr_read，地址为 right，长度为一个 word
        return paddr_read(right, sizeof(word_t));
      default: *ok = false; return 0;
    } 
  }
  switch (op_type) {                                                     // 双目运算符
    case TK_ADD: return left + right;
    case TK_SUB: return left - right;
    case TK_MUL: return left * right;
    case TK_DIV:
      if (right == 0) {
        *ok = false;
        printf("除以零错误\n");
        return 0;
      }
      /*printf("left = %u, right = %u, result = %u\n", left, right, left/right); */  //要显示除法就把这里开了
      return left / right;
    case TK_EQ:  return (left == right) ? 1 : 0;
    case TK_NEQ: return (left != right) ? 1 : 0;
    case TK_AND: return (left && right) ? 1 : 0;
    default: *ok = false; return 0;
  }
}

// 运算优先级功能函数：根据 token_type 返回优先级数值，越大优先级越高
int priority(int token_type) {
  if (token_type == TK_NEG || token_type == TK_DEREF) return 4;        // 单目优先级最高
  if (token_type == TK_MUL || token_type == TK_DIV) return 3;          // 乘除
  if (token_type == TK_ADD || token_type == TK_SUB) return 2;          // 加减
  if (token_type == TK_EQ || token_type == TK_NEQ) return 1;           // 比较
  if (token_type == TK_AND) return 0;                                  // 逻辑与最低
  return 0;  // 其他（如左括号）
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  //======备忘录============//
  //上面是判断maketoken是否成功的，不管
  //maketoken处理后的token都在 tokens【】数组里，每个成员有 tokens【】。type和 str。
  //======备忘录结束======//

    uint32_t num_stack[1024]; //存数字 - 扩大为128
    int op_stack[1024]; //存符号类型 - 扩大为128
    int num_top = -1, op_top = -1;

    for(int i = 0; i < nr_token; i++){//遍历tokens数组
      int type = tokens[i].type;
      if(type == TK_DEC){ // 十进制数字
        errno = 0;
        unsigned long val = strtoul(tokens[i].str, NULL, 10);  //转无符号  strtoul（要被转的字符串 终止指针 进制）
        if (errno == ERANGE) {*success = false; printf("数字超出范围\n"); return 0;}
        num_top++;
        num_stack[num_top] = (uint32_t)val; //压栈
      }
      else if(type == TK_HEX){ // 十六进制数字
        errno = 0;
        unsigned long val = strtoul(tokens[i].str, NULL, 16);
        if (errno == ERANGE) {*success = false; printf("十六进制数超出范围\n"); return 0;}
        num_top++;
        num_stack[num_top] = (uint32_t)val;
      }
      else if(type == TK_REG){ // 寄存器
        bool reg_ok;
        word_t val = isa_reg_str2val(tokens[i].str, &reg_ok); // 传入不带$的名字
        if (!reg_ok) { *success = false; return 0; }
        num_top++;
        num_stack[num_top] = val;
      }
      else if(type == TK_LPAREN) {  //左括号
        op_top++;
        op_stack[op_top] = type;
      }
      else if(type == TK_RPAREN){  //右括号 开始弹出
        while (op_top >= 0 && op_stack[op_top] != TK_LPAREN) { //备忘：栈没空且栈顶不是左括号
          bool ok = true;
          uint32_t result;
          int curr_op = op_stack[op_top];
          // 处理单目运算符（负号、解引用）
          if (curr_op == TK_NEG || curr_op == TK_DEREF) {
            if(num_top < 0){                              //数值栈内无操作数（单目运算需要1个）
              *success = false;
              return 0;
            }
            uint32_t val = num_stack[num_top--];  //弹出数值栈
            result = apply_op(0, curr_op, val, &ok, true); 
          }
          // 处理双目运算符（包括新添加的比较、逻辑等）
          else {
            if(num_top < 1){                              //数值栈内的数量不够完成一次双目运算，就是出错
              *success = false;
              return 0;
            }
            uint32_t right = num_stack[num_top--];
            uint32_t left = num_stack[num_top--];
            result = apply_op(left, curr_op, right, &ok, false);
          }

          if(ok == false){
            *success = false;
            return 0;
          }
          num_top ++;                                   //中间运算结果压栈
          num_stack[num_top] = result;
          op_top--;
        }
        if(op_top < 0){                              //此时还有左括号残留，如果空了就是出问题了
          *success = false; 
          return 0; 
        }
        op_top--; // 弹出左括号，到此就消掉了一层括号
      }
      // 处理运算符（包括所有单目和双目）
      else if(type == TK_ADD || type == TK_SUB || type == TK_MUL || type == TK_DIV || 
              type == TK_NEG || type == TK_DEREF || type == TK_EQ || type == TK_NEQ || type == TK_AND) { //是运算符，这里要处理一下优先级
        while(op_top >= 0 && op_stack[op_top] != TK_LPAREN){//备忘：栈没空且栈顶不是左括号，然后判断优先级
          int stack_pri = priority(op_stack[op_top]);
          int cur_pri = priority(type);
          int should_pop = 0;
          // 单目运算符（右结合）特殊处理
          if(type == TK_NEG || type == TK_DEREF){
            if(stack_pri > cur_pri){ //不能等于是因为 假设 - - 5，第一个负号进了，第二个要等操作数
              should_pop = 1;
            }
          } 
          else { // 双目运算符（左结合）
            if(stack_pri >= cur_pri){
              should_pop = 1;
            }
          }
          if(should_pop == 0){
            break;         
          }

          bool ok = true;
          uint32_t result;
          int curr_op = op_stack[op_top];

          // 处理单目
          if (curr_op == TK_NEG || curr_op == TK_DEREF) {
            if(num_top < 0){                              //数值栈内无操作数
              *success = false;
              return 0;
            }
            uint32_t val = num_stack[num_top--];
            result = apply_op(0, curr_op, val, &ok, true);
          }
          // 处理双目
          else {
            if(num_top < 1){
              *success = false;
              return 0;
            }
            uint32_t right = num_stack[num_top--];
            uint32_t left = num_stack[num_top--];
            result = apply_op(left, curr_op, right, &ok, false);
          }

          if(ok == false){
            *success = false;
            return 0;
          }
          num_top ++;                                   //中间运算结果压栈
          num_stack[num_top] = result;
          op_top--;
        }
        op_top++;
        op_stack[op_top] = type; //压操作（存token_type）
      }
      else {  //什么符号都不是
        *success = false;
        return 0;
      }
    }

    //遍历完了，需要立即计算的也搞完了，留下的就是没括号的一层表达式求值了
    while (op_top >= 0) {                               
      bool ok = true;
      uint32_t result;
      int curr_op = op_stack[op_top];

      // 处理单目
      if (curr_op == TK_NEG || curr_op == TK_DEREF) {
        if(num_top < 0){                              //数值栈内无操作数
          *success = false;
          return 0;
        }
        uint32_t val = num_stack[num_top--];
        result = apply_op(0, curr_op, val, &ok, true);
      }
      // 处理双目
      else {
        if (num_top < 1) { 
          *success = false; 
          return 0; 
        }
        uint32_t right = num_stack[num_top--];
        uint32_t left = num_stack[num_top--];
        result = apply_op(left, curr_op, right, &ok, false);
      }

      if (ok == false) { 
        *success = false; 
        return 0; 
      }
      num_top++;
      num_stack[num_top] = result;
      op_top--;
    }

    //此时符号栈处理完了，如果数字栈没空那就是出错了
    if (num_top != 0) {
      *success = false;
      return 0; 
    }
    *success = true;
    return num_stack[0]; //返回最终数值无需强转，已为uint32_t
}