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
  TK_RPAREN         // 右括号 ) 
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  {"[0-9]+u", TK_DEC},  //处理带u后置的
  {"[0-9]+", TK_DEC},        
  {"\\+", TK_ADD},           
  {"\\-", TK_SUB},           
  {"\\*", TK_MUL},           
  {"\\/", TK_DIV},           
  {"\\(", TK_LPAREN},        
  {"\\)", TK_RPAREN},        
  {" +", TK_NOTYPE},         
  {"==", TK_EQ},      
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
        int substr_len = pmatch.rm_eo;               //匹配到的子串的长度是 eo-so = eo-0 = eo 你滴 明白？

        /*Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start); */

        position += substr_len;  //然后把position偏移量往后挪 之前匹配到的字符串的长度/

        if (rules[i].token_type != TK_NOTYPE) {  //不是空格就存
          if(nr_token >= 1023){   
            printf("token数组越界，保证输入小于1023个字符\n");
            return false;
          }
          int copy_len = substr_len;
          // 如果是数字类型，且末尾有 'u'，则去掉它
          if (rules[i].token_type == TK_DEC && copy_len > 0 && substr_start[copy_len-1] == 'u') {
              copy_len--;
          }
          if(copy_len >= 127){
            printf("单个字符过长，保证单字符串小于128字符\n");
            return false;
          }
          strncpy(tokens[nr_token].str, substr_start, copy_len);  // 目标数组 要复制的字符串起点，复制的数量，复习
          tokens[nr_token].str[copy_len] = '\0';
          tokens[nr_token].type = rules[i].token_type;

          nr_token++; // 计数+1
          }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  //重新遍历一遍，区分负号和减号，上面是统一识别为减号的
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
  }
  return true;
}

//求值部分

//运算+防除以零情况功能函数
// 修改：适配uint32_t类型，支持单目负号（无符号取反：0 - right）
uint32_t apply_op(uint32_t left, char op, uint32_t right, bool *ok, bool is_unary) {
  if(is_unary == true){
    if(op == '-'){
      return 0 - right;
    }                                                               //《《《《《=================================================在这里补其他单目运算符
    *ok = false;
    return 0;
  }
  switch (op) {                                                     //《《《《《《《=====================================在这里的分支里补双目和对应逻辑
    case '+': return left + right;
    case '-': return left - right;
    case '*': return left * right;
    case '/':
      if (right == 0) {
        *ok = false;
        printf("除以零错误\n");
        return 0;
      }
      printf("left = %u, right = %u, result = %u\n", left, right, left/right);
      return left / right;
    default:
      *ok = false;
      return 0;
  }
}

// 运算优先级功能函数：修改为接收token_type，负号优先级最高（3），乘除2，加减1
int priority(int token_type) {
  if (token_type == TK_NEG) return 3;        // 单目负号优先级最高
  if (token_type == TK_ADD || token_type == TK_SUB) return 1;
  if (token_type == TK_MUL || token_type == TK_DIV) return 2;
  return 0;
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
      if(type == TK_DEC){ // 是数字就压到数字栈
        
        errno = 0;
        unsigned long val = strtoul(tokens[i].str, NULL, 10);  //转无符号  strtoul（要被转的字符串 终止指针 进制）

        if (errno == ERANGE) {
          *success = false;
          printf("数字超出范围\n");
          return 0;
        }

        num_top++;
        num_stack[num_top] = (uint32_t)val; //压栈

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

          // 处理单目负号
          if (curr_op == TK_NEG) {
            if(num_top < 0){                              //数值栈内无操作数（单目运算需要1个）
              *success = false;
              return 0;
            }
            uint32_t val = num_stack[num_top--];
            result = apply_op(0, '-', val, &ok, true);
          }
          // 处理双目运算符
          else {
            if(num_top < 1){                              //数值栈内的数量不够完成一次双目运算，就是出错
              *success = false;
              return 0;
            }
            uint32_t right = num_stack[num_top--];
            uint32_t left = num_stack[num_top--];
            char op = 0;
            switch (curr_op) {                     //先给运算赋值
              case TK_ADD: op = '+'; break;
              case TK_SUB: op = '-'; break;
              case TK_MUL: op = '*'; break;
              case TK_DIV: op = '/'; break;
              default: op = 0;
            }
            result = apply_op(left, op, right, &ok, false);
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
      // 处理运算符（包括TK_NEG）
      else if(type == TK_ADD || type == TK_SUB || type == TK_MUL || type == TK_DIV || type == TK_NEG) { //是运算符，这里要处理一下优先级
        while(op_top >= 0 && op_stack[op_top] != TK_LPAREN){//备忘：栈没空且栈顶不是左括号，然后判断优先级
          int stack_pri = priority(op_stack[op_top]);
          int cur_pri = priority(type);
          int should_pop = 0;
          if(type == TK_NEG){
          //单目右结合，单拎出来
            if(stack_pri > cur_pri){
              should_pop = 1;
            }
          } else {
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

          // 处理单目负号
          if (curr_op == TK_NEG) {
            if(num_top < 0){                              //数值栈内无操作数（单目运算需要1个）
              *success = false;
              return 0;
            }
            uint32_t val = num_stack[num_top--];
            result = apply_op(0, '-', val, &ok, true);
          }
          // 处理双目运算符
          else {
            if(num_top < 1){                              //数值栈内的数量不够完成一次双目运算，就是出错
              *success = false;
              return 0;
            }
            uint32_t right = num_stack[num_top--];
            uint32_t left = num_stack[num_top--];
            char op = 0;
            switch (curr_op) {                     //先给运算赋值
              case TK_ADD: op = '+'; break;
              case TK_SUB: op = '-'; break;
              case TK_MUL: op = '*'; break;
              case TK_DIV: op = '/'; break;
              default: op = 0;
            }
            result = apply_op(left, op, right, &ok, false);
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
        op_stack[op_top] = type; //压操作（存token_type而非char）
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

      // 处理单目负号
      if (curr_op == TK_NEG) {
        if(num_top < 0){                              //数值栈内无操作数（单目运算需要1个）
          *success = false;
          return 0;
        }
        uint32_t val = num_stack[num_top--];
        result = apply_op(0, '-', val, &ok, true);
      }
      // 处理双目运算符
      else {
        if (num_top < 1) { 
          *success = false; 
          return 0; 
        }
        uint32_t right = num_stack[num_top--];
        uint32_t left = num_stack[num_top--];
        char op = 0;
        switch (curr_op) {                     //先给运算赋值
          case TK_ADD: op = '+'; break;
          case TK_SUB: op = '-'; break;
          case TK_MUL: op = '*'; break;
          case TK_DIV: op = '/'; break;
          default: op = 0;
        }
        result = apply_op(left, op, right, &ok, false);
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