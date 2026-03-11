#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <stdbool.h>

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

enum {
  TK_NOTYPE = 256,
  TK_EQ,
  TK_DEC,
  TK_ADD,
  TK_SUB,
  TK_NEG,
  TK_MUL,
  TK_DIV,
  TK_LPAREN,
  TK_RPAREN
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
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
static regex_t re[NR_REGEX];

static void init_regex() {
  for (int i = 0; i < NR_REGEX; i++) {
    int ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      fprintf(stderr, "regex compilation failed: %s\n", rules[i].regex);
      exit(1);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[128];
static int nr_token = 0;

static bool make_token(char *e) {
  int position = 0;
  nr_token = 0;
  while (e[position] != '\0') {
    for (int i = 0; i < NR_REGEX; i++) {
      regmatch_t pmatch;
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
        position += substr_len;
        if (rules[i].token_type != TK_NOTYPE) {
          if (nr_token >= 128) {
            printf("token array overflow\n");
            return false;
          }
          tokens[nr_token].type = rules[i].token_type;
          int len = substr_len < 31 ? substr_len : 31;
          strncpy(tokens[nr_token].str, substr_start, len);
          tokens[nr_token].str[len] = '\0';
          nr_token++;
        }
        break;
      }
    }
  }
  for (int j = 0; j < nr_token; j++) {
    if (tokens[j].type == TK_SUB) {
      if (j == 0 ||
          tokens[j-1].type == TK_ADD || tokens[j-1].type == TK_SUB ||
          tokens[j-1].type == TK_MUL || tokens[j-1].type == TK_DIV ||
          tokens[j-1].type == TK_LPAREN || tokens[j-1].type == TK_NEG) {
        tokens[j].type = TK_NEG;
      }
    }
  }
  return true;
}

static uint32_t apply_op(uint32_t left, char op, uint32_t right, bool *ok, bool is_unary) {
  if (is_unary) {
    if (op == '-') return 0 - right;
    *ok = false;
    return 0;
  }
  switch (op) {
    case '+': return left + right;
    case '-': return left - right;
    case '*': return left * right;
    case '/':
      if (right == 0) {
        *ok = false;
        return 0;
      }
      return left / right;
    default: *ok = false; return 0;
  }
}

static int priority(int token_type) {
  if (token_type == TK_NEG) return 3;
  if (token_type == TK_ADD || token_type == TK_SUB) return 1;
  if (token_type == TK_MUL || token_type == TK_DIV) return 2;
  return 0;
}

static uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  uint32_t num_stack[1024];
  int op_stack[1024];
  int num_top = -1, op_top = -1;

  for (int i = 0; i < nr_token; i++) {
    int type = tokens[i].type;
    if (type == TK_DEC) {
      num_top++;
      errno = 0;
      unsigned long val = strtoul(tokens[i].str, NULL, 10);
      if (errno == ERANGE) {
        *success = false;
        return 0;
      }
      num_stack[num_top] = (uint32_t)val;
    } else if (type == TK_LPAREN) {
      op_stack[++op_top] = type;
    } else if (type == TK_RPAREN) {
      while (op_top >= 0 && op_stack[op_top] != TK_LPAREN) {
        bool ok = true;
        uint32_t result;
        int curr_op = op_stack[op_top];
        if (curr_op == TK_NEG) {
          if (num_top < 0) { *success = false; return 0; }
          uint32_t val = num_stack[num_top--];
          result = apply_op(0, '-', val, &ok, true);
        } else {
          if (num_top < 1) { *success = false; return 0; }
          uint32_t right = num_stack[num_top--];
          uint32_t left = num_stack[num_top--];
          char op = 0;
          switch (curr_op) {
            case TK_ADD: op = '+'; break;
            case TK_SUB: op = '-'; break;
            case TK_MUL: op = '*'; break;
            case TK_DIV: op = '/'; break;
            default: op = 0;
          }
          result = apply_op(left, op, right, &ok, false);
        }
        if (!ok) { *success = false; return 0; }
        num_stack[++num_top] = result;
        op_top--;
      }
      if (op_top < 0) { *success = false; return 0; }
      op_top--;
    } else if (type == TK_ADD || type == TK_SUB || type == TK_MUL || type == TK_DIV || type == TK_NEG) {
      while (op_top >= 0 && op_stack[op_top] != TK_LPAREN) {
        int stack_pri = priority(op_stack[op_top]);
        int cur_pri = priority(type);
        int should_pop = 0;
        if (type == TK_NEG) {
          if (stack_pri > cur_pri) should_pop = 1;
        } else {
          if (stack_pri >= cur_pri) should_pop = 1;
        }
        if (!should_pop) break;

        bool ok = true;
        uint32_t result;
        int curr_op = op_stack[op_top];
        if (curr_op == TK_NEG) {
          if (num_top < 0) { *success = false; return 0; }
          uint32_t val = num_stack[num_top--];
          result = apply_op(0, '-', val, &ok, true);
        } else {
          if (num_top < 1) { *success = false; return 0; }
          uint32_t right = num_stack[num_top--];
          uint32_t left = num_stack[num_top--];
          char op = 0;
          switch (curr_op) {
            case TK_ADD: op = '+'; break;
            case TK_SUB: op = '-'; break;
            case TK_MUL: op = '*'; break;
            case TK_DIV: op = '/'; break;
            default: op = 0;
          }
          result = apply_op(left, op, right, &ok, false);
        }
        if (!ok) { *success = false; return 0; }
        num_stack[++num_top] = result;
        op_top--;
      }
      op_stack[++op_top] = type;
    } else {
      *success = false;
      return 0;
    }
  }

  while (op_top >= 0) {
    bool ok = true;
    uint32_t result;
    int curr_op = op_stack[op_top];
    if (curr_op == TK_NEG) {
      if (num_top < 0) { *success = false; return 0; }
      uint32_t val = num_stack[num_top--];
      result = apply_op(0, '-', val, &ok, true);
    } else {
      if (num_top < 1) { *success = false; return 0; }
      uint32_t right = num_stack[num_top--];
      uint32_t left = num_stack[num_top--];
      char op = 0;
      switch (curr_op) {
        case TK_ADD: op = '+'; break;
        case TK_SUB: op = '-'; break;
        case TK_MUL: op = '*'; break;
        case TK_DIV: op = '/'; break;
        default: op = 0;
      }
      result = apply_op(left, op, right, &ok, false);
    }
    if (!ok) { *success = false; return 0; }
    num_stack[++num_top] = result;
    op_top--;
  }

  if (num_top != 0) {
    *success = false;
    return 0;
  }
  *success = true;
  return num_stack[0];
}

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

// ------------------ 辅助函数：生成随机无符号数 ------------------
static void gen_num() {
  // 生成一个32位的随机无符号整数
  unsigned val = ((unsigned)rand() << 16) ^ (unsigned)rand();
  char tmp[20];                        // 足够存放10位数字 + 'u' + '\0'
  sprintf(tmp, "%u", val);              // 将数值转为十进制字符串
  int len = strlen(tmp);
  tmp[len] = 'u';                       // 添加 'u' 后缀，强制为无符号常量
  tmp[len + 1] = '\0';
  strcat(buf, tmp);                     // 追加到全局缓冲区
}

// ------------------ 辅助函数：随机生成运算符 ------------------
static char gen_rand_op() {
  int op = rand() % 4;                   // 0,1,2,3
  switch (op) {
    case 0: return '+';
    case 1: return '-';
    case 2: return '*';
    default: return '/';
  }
}

// ------------------ 辅助函数：向缓冲区添加单个字符 ------------------
static void gen(char c) {
  char str[2] = {c, '\0'};
  strcat(buf, str);
}

// ------------------ 辅助函数：随机插入空格 ------------------
static void gen_space() {
  // 以30%的概率插入空格
  if (rand() % 10 < 3) {
    int n = rand() % 3 + 1;              // 随机1~3个空格
    for (int i = 0; i < n; i++) {
      strcat(buf, " ");
    }
  }
}

// ------------------ 核心递归生成函数 ------------------
// depth 控制递归深度，防止无限递归和缓冲区溢出
static void gen_expr(int depth) {
  // 如果深度超过8，强制生成数字（递归终止条件）
  if (depth > 8) {
    gen_num();
    return;
  }

  int choice = rand() % 3;               // 随机选择三种生成方式之一
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

    default: // 生成二元运算: expr op expr
      gen_expr(depth + 1);   // 左操作数
      gen_space();
      char op = gen_rand_op();
      // 避免出现连续两个减号（--），这会被C语言解释为自减运算符
      if (buf[0] != '\0' && buf[strlen(buf)-1] == '-' && op == '-') {
        strcat(buf, " ");    // 插入一个空格分隔
      }
      gen(op);               // 添加运算符
      gen_space();
      gen_expr(depth + 1);   // 右操作数
      break;
  }
}

// ------------------ 对外接口：生成随机表达式 ------------------
static void gen_rand_expr() {
  buf[0] = '\0';        // 清空缓冲区
  gen_expr(0);          // 从深度0开始递归
}

// ------------------ 主函数 ------------------
int main(int argc, char *argv[]) {
  // 初始化正则表达式（expr函数需要）
  init_regex();

  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);   // 从命令行获取要生成的表达式数量
  }

  int total = 0, passed = 0;
  for (int i = 0; i < loop; i++) {
    gen_rand_expr();                 // 生成随机表达式到 buf

    // 将表达式填入模板，得到完整的C程序
    sprintf(code_buf, code_format, buf);

    // 将C程序写入临时文件
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // 编译临时文件，生成可执行文件 /tmp/.expr
    int ret = system("gcc /tmp/.code.c -o /tmp/.expr 2>/dev/null");
    if (ret != 0) continue;           // 编译失败则跳过该表达式

    // 运行可执行文件，并读取其输出
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    unsigned expected;                   // 用unsigned存储结果
    ret = fscanf(fp, "%u", &expected);
    pclose(fp);
    if (ret != 1) continue;               // 读取失败则跳过

    // 调用你自己的expr函数求值
    bool success;
    unsigned actual = (unsigned)expr(buf, &success);

    total++;
    if (success && actual == expected) {
      passed++;
    } else {
      printf("FAIL: %s => expected %u, got %u (success=%d)\n",
             buf, expected, actual, success);
    }
  }
  printf("Total: %d, Passed: %d, Failed: %d\n", total, passed, total - passed);
  return 0;
}