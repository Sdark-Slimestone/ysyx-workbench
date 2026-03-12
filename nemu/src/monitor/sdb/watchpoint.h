#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <stdint.h>
#include <stdbool.h>

// 监视点结构体的前置声明
typedef struct watchpoint {
  int NO;
  uint32_t result;
  bool result_en;
  struct watchpoint *next;
  char exp[114514];
} WP;


// 初始化监视点池
void init_wp_pool();

// 创建一个新的监视点，返回指向WP的指针
WP* new_wp(const char *exp);

// 释放指定的监视点
void free_wp(WP *wp);

// 检查所有监视点的值是否变化，若变化返回true
bool check_watchpoints();

// 打印所有使用中的监视点信息
void info_watchpoints();

// 根据编号删除监视点（用于d命令）
void delete_watchpoint(int no);

#endif