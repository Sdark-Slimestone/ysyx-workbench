#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
    static char *heap_ptr = NULL;
    if (heap_ptr == NULL) {
        heap_ptr = (char *)heap.start;   // 使用 AM 的堆起始地址:/home/sdark/ysyx-workbench/abstract-machine/am/include/am.h
    }
    size = (size + 7) & ~7;              // 8字节对齐
    if (heap_ptr + size > (char *)heap.end) {
        return NULL;                     // 堆空间不足
    }
    void *ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}

void free(void *ptr) {
    // 空实现
}


#endif
