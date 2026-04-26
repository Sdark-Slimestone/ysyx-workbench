#ifndef __IRINGBUF_H__
#define __IRINGBUF_H__

#include <stdint.h>
#include <memory/paddr.h>  

#define IRINGBUF_SIZE          16    // 缓冲区容量，可按需调整 
#define IRINGBUF_INST_LEN      4     // RV32

// 单条
typedef struct {
  paddr_t pc;                     // 该指令的物理地址 
  uint8_t inst[IRINGBUF_INST_LEN]; 
} IRingEntry;

/* 环形缓冲区 */
typedef struct {
  IRingEntry entries[IRINGBUF_SIZE];
  int        next;    // 下一次写入的位置 
  int        count;   // 已写入的有效条目数（最大为 SIZE）
} IRingBuf;

void iringbuf_init(IRingBuf *rb);
void iringbuf_push(IRingBuf *rb, paddr_t pc, const uint8_t *inst);
void iringbuf_dump(IRingBuf *rb, paddr_t fault_pc);

#endif