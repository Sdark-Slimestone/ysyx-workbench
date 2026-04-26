#include <common.h>
#include "iringbuf.h"


void iringbuf_init(IRingBuf *rb) {
  rb->next  = 0;
  rb->count = 0;
}

void iringbuf_push(IRingBuf *rb, paddr_t pc, const uint8_t *inst) {
  int write_pos = rb->next;   // 这次要写入的格子索引

  // 直接填入数据
  rb->entries[write_pos].pc = pc;
  for (int i = 0; i < IRINGBUF_INST_LEN; i++) {
    rb->entries[write_pos].inst[i] = inst[i];
  }

  // 写指针前进
  if (write_pos + 1 >= IRINGBUF_SIZE) {
    rb->next = 0;   // 到末尾了，回到开头
  } else {
    rb->next = write_pos + 1;   // 否则直接往后移
  }

  // 有效记录数增加
  if (rb->count < IRINGBUF_SIZE) {
    rb->count++;
  }
}

void iringbuf_dump(IRingBuf *rb, paddr_t fault_pc) {
  if (rb->count == 0) {
    printf("iringbuf: no instructions recorded.\n");
    return;
  }

  printf("========== IRINGBUF DUMP ==========\n");

  int valid = rb->count;
  int start;

  // 直接判断最旧记录的起始下标
  if (rb->count < IRINGBUF_SIZE) {
    start = 0;
  } else {
    start = rb->next;
  }

  int idx = start;   // 从最旧记录的下标开始
  for (int i = 0; i < valid; i++) {
    // 标记出错指令
    if (rb->entries[idx].pc == fault_pc) {
      printf(" --> ");
    } else {
      printf("     ");
    }

    printf(FMT_WORD ": ", rb->entries[idx].pc);

    #ifdef CONFIG_ITRACE
    // 只在 ITRACE 开启时声明并调用 disassemble
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
    char disasm[128] = {0};
    disassemble(disasm, sizeof(disasm), rb->entries[idx].pc, rb->entries[idx].inst, IRINGBUF_INST_LEN);
    printf("%s", disasm);
    #else
    printf("????"); 
    #endif

    printf("   ");
    for (int j = 0; j < IRINGBUF_INST_LEN; j++) {
      printf(" %02x", rb->entries[idx].inst[j]);
    }
    printf("\n");

    // 移动到下一个格子，环回
    if (idx + 1 >= IRINGBUF_SIZE) {
      idx = 0;
    } else {
      idx = idx + 1;
    }
  }

  printf("====================================\n");
}