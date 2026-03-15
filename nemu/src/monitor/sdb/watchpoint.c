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

#include "sdb.h"
#include "assert.h"
#include "expr.h"
#include <stdint.h>
#include <string.h> 
#include "watchpoint.h"

#define NR_WP 32
//是个双单向链表结构，需要监视点的时候就会从一个free_链表中取一个节点下来，放到另一个链表head上，反之亦然
/*限制作用域防止冲突，不用手动分配和释放*/
static WP wp_pool[NR_WP] = {};          
static WP *head = NULL, *free_ = NULL;  //head是正在使用的链表。初始化时是空的，free是空链表，初始化一堆空元素

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);  //最后一个链表的next需要为空
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(const char* exp) {
  //参数检查
  if (exp == NULL) {
    printf("错误：表达式为空\n");
    return NULL;
  }

  //检查空闲节点
  if (free_ == NULL) {
    printf("错误：无空闲监视点，请先删除一些\n");
    return NULL;
  }

  //检查表达式长度
  size_t exp_len = strlen(exp);
  if (exp_len >= sizeof(wp_pool[0].exp)) {
    printf("错误：表达式过长（最大 %zu 字符）\n", sizeof(wp_pool[0].exp) - 1);
    return NULL;
  }

  //表达式求值
  bool success = false;
  uint32_t val = (uint32_t)expr((char *)exp, &success);
  if (!success) {
    printf("错误：表达式求值失败，表达式 = \"%s\"\n", exp);
    return NULL;
  }

  //从空闲链表取下节点，此时所有失败条件已排除
  WP *gained = free_;
  free_ = free_->next;      // 更新空闲链表头
  gained->next = NULL;      // 断开与原链表的联系

  //初始化节点内容
  gained->result = val;
  gained->result_en = true;
  strncpy(gained->exp, exp, sizeof(gained->exp) - 1);
  gained->exp[sizeof(gained->exp) - 1] = '\0';

  //将节点插入head链表
  if (head == NULL) {
    head = gained;
  } else {
    WP *tail = head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = gained;
  }

  return gained;
}


void free_wp(WP *wp){
  if(wp == NULL){
    printf("监视点呢？\n");
    assert(0);
  }
  /*从head里删链表*/
  if(head == wp){                              //要删除的是第一个节点
    head = wp->next;
    wp->next = NULL;
  }
  else {
    WP *check_next = head;
    while(check_next->next != wp && check_next->next != NULL){                    //当被找到节点的下一个节点是wp
      check_next = check_next->next;
    }
    if(check_next->next == NULL){
      printf("不存在该监视点\n");
      assert(0);
    }
    if(wp->next == NULL){                            //wp是head的最后一个节点
      wp->next = NULL;
      check_next->next = NULL;
    }
    else {                                          //wp是中间的节点
      WP *node_tobe_connected = wp->next;
      wp->next = NULL;                             //先只做管理，内容后面再说
      check_next->next = node_tobe_connected;
    }
  }
  /*清空内容*/
  wp->result = 0;
  wp->result_en = false;
  memset(wp->exp, 0, sizeof(wp->exp)); // 清空表达式数组（填0）

  /*把链表接回free，这里用头插*/
  //如果free是空的
  if(free_ == NULL){
    free_ = wp;
  }
  //如果free不空
  else{
    WP *node_tobe_connected = free_;
    free_ = wp;
    wp->next = node_tobe_connected;
  }
}

bool check_watchpoints() {

  if (head == NULL) {
      printf("当前没有监视点\n");
      return false;
  }
  
  WP *wp = head;
  int triggered = 0;   // 标记是否有监视点被触发

  while (wp != NULL) {
    bool success;
    uint32_t new_val = expr(wp->exp, &success);
    if (!success) {
        //如果表达式失败
        printf("警告：监视点 %d 表达式求值失败，后续不会触发监视点\n", wp->NO);
        wp = wp->next;
        continue;
    }
    if (new_val != wp->result) {
        printf("监视点 %d: %s\n", wp->NO, wp->exp);
        printf("旧值: %u (0x%x)\n", wp->result, wp->result);
        printf("新值: %u (0x%x)\n", new_val, new_val);
        wp->result = new_val;   // 更新保存的值
        triggered = 1;
    }
    wp = wp->next;
  }
  return triggered;
}

/* 打印所有使用中的监视点信息 */
void info_watchpoints() {
  if (head == NULL) {
    printf("没有监视点\n");
    return;
  }
  printf("编号\t表达式\t\t当前值\n");
  WP *wp = head;
  while (wp != NULL) {
    printf("%d\t%s\t\t%u (0x%x)\n", wp->NO, wp->exp, wp->result, wp->result);
    wp = wp->next;
  }
}

/* 根据编号删除监视点 */
void delete_watchpoint(int no) {
  WP *wp = head;
  while (wp != NULL) {
    if (wp->NO == no) {
        free_wp(wp);
        return;
    }
    wp = wp->next;
  }
  printf("未找到编号为 %d 的监视点\n", no);
}