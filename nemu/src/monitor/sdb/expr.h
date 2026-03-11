#ifndef __EXPR_H__
#define __EXPR_H__

#include <common.h>   // 包含 word_t, bool 等类型定义

word_t expr(char *e, bool *success);
void init_regex(void);

#endif