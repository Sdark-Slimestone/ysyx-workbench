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

static char buf[65536] = {};
static char code_buf[65536 + 128] = {};
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

// 判断运算符是否需要强制转换为 unsigned（逻辑与比较运算符）
static int need_cast(const char *op) {
    return strcmp(op, "&&") == 0 || strcmp(op, "==") == 0 || strcmp(op, "!=") == 0;
}

static void gen_num() {
    if (rand() % 2 == 0) {
        uint32_t val = (uint32_t)(((uint32_t)rand()) << (rand() % 16));
        char tmp[20];
        sprintf(tmp, "%u", val);
        int len = strlen(tmp);
        tmp[len] = 'u';
        tmp[len + 1] = '\0';
        strcat(buf, tmp);
    } else {
        unsigned val = (uint32_t)(((uint32_t)rand()) << (rand() % 16));
        char tmp[20];
        sprintf(tmp, "0x%X", val);
        int len = strlen(tmp);
        tmp[len] = 'u';
        tmp[len + 1] = '\0';
        strcat(buf, tmp);
    }
}

static const char* gen_rand_op_str() {
    int op = rand() % 7;
    switch (op) {
        case 0: return "+";
        case 1: return "-";
        case 2: return "*";
        case 3: return "/";
        case 4: return "==";
        case 5: return "!=";
        default: return "&&";
    }
}

static void gen(char c) {
    char str[2] = {c, '\0'};
    strcat(buf, str);
}

static void gen_space() {
    if (rand() % 10 < 3) {
        int n = rand() % 3 + 1;
        for (int i = 0; i < n; i++) {
            strcat(buf, " ");
        }
    }
}

static void gen_expr(int depth) {
    if (depth > 6) {
        gen_num();
        return;
    }

    int choice = rand() % 4;
    switch (choice) {
        case 0:
            gen_space();
            gen_num();
            gen_space();
            break;
        case 1:
            gen_space();
            gen('(');
            gen_space();
            gen_expr(depth + 1);
            gen_space();
            gen(')');
            gen_space();
            break;
        case 2:
            gen_space();
            if (buf[0] != '\0' && buf[strlen(buf) - 1] == '-') {
                strcat(buf, " ");
            }
            gen('-');
            gen('(');
            gen_space();
            gen_expr(depth + 1);
            gen_space();
            gen(')');
            break;
        default: {
            // 先获取运算符
            const char* op = gen_rand_op_str();
            // 判断是否需要强制转换
            if (need_cast(op)) {
                // 对于需要转换的运算符，整体用 (unsigned)( ... ) 包裹
                strcat(buf, "(unsigned)(");
                gen_expr(depth + 1);   // 左操作数
                gen_space();
                // 避免连续减号
                if (strcmp(op, "-") == 0 && buf[0] != '\0' && buf[strlen(buf)-1] == '-') {
                    strcat(buf, " ");
                }
                strcat(buf, op);
                gen_space();
                gen_expr(depth + 1);   // 右操作数
                strcat(buf, ")");
            } else {
                // 普通算术运算符，直接生成
                gen_expr(depth + 1);
                gen_space();
                if (strcmp(op, "-") == 0 && buf[0] != '\0' && buf[strlen(buf)-1] == '-') {
                    strcat(buf, " ");
                }
                strcat(buf, op);
                gen_space();
                gen_expr(depth + 1);
            }
            break;
        }
    }
}

static void gen_rand_expr() {
    buf[0] = '\0';
    gen_expr(0);
}

int main(int argc, char *argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &loop);
    }

    for (int i = 0; i < loop; i++) {
        gen_rand_expr();
        sprintf(code_buf, code_format, buf); //把表达式写入模板

        FILE *fp = fopen("/tmp/.code.c", "w");  //打开一个临时文件，把完整的代码写进去
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        char cmd[256];
        snprintf(cmd, sizeof(cmd), "gcc /tmp/.code.c -o /tmp/.expr 2>&1");  //把命令存入cmd数组
        FILE *compile_fp = popen(cmd, "r");                                 //启动子进程，输出到屏幕的同时结果写入管道
        if (compile_fp == NULL) {                                           
            continue;
        }
        char output[1024] = {0};
        size_t n = fread(output, 1, sizeof(output)-1, compile_fp);          //把管道里的输出读出来
        (void)n;
        int ret = pclose(compile_fp);                                         
        if (ret != 0 || strstr(output, "division by zero") != NULL) {  //检查除以0警告
            continue;                                                  
        }

        fp = popen("/tmp/.expr", "r");
        assert(fp != NULL);

        unsigned result;
        int scanned = fscanf(fp, "%u", &result);
        int status = pclose(fp);
        if (scanned != 1 || status != 0) {
            continue;
        }
        printf("%u %s\n", result, buf); //最后决定输出文本格式的就是这里
    }
    return 0;
}