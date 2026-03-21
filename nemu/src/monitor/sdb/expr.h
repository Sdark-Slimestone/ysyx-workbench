#ifndef __EXPR_H__
#define __EXPR_H__

#include <common.h>   
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

word_t expr(char *e, bool *success);
void init_regex(void);

#endif