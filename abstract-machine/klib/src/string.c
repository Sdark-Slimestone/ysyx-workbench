#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t i = 0;
  for(const char *p = s; *p != '\0'; p++){
    i++;
  }
  return i;
}

char *strcpy(char *dst, const char *src) {
  char *dst_pre = dst;
  size_t size = strlen(src);
  for(size_t i = 0; i <= size; i++){
    *dst = *src;
    dst++;
    src++;
  }
  return dst_pre;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *dst_pre = dst;
  size_t size = strlen(src);
  if(size < n){
    for(size_t i = 1; i <= n; i++){
      if(i <= size){
        *dst = *src;
        dst++;
        src++;
      }
      else{
        *dst = '\0';
        dst++;
      }
    }
    return dst_pre;
  }
  else {
    for(size_t i = 1; i <= n; i++){
      *dst = *src;
      dst++;
      src++;
    }
    return dst_pre;
  }
}

char *strcat(char *dst, const char *src) {
  char *dst_pre = dst;
  size_t size = strlen(dst);
  strcpy((dst+size), src);
  return dst_pre;
}

int strcmp(const char *s1, const char *s2) {
  int t;
  while(*s1 == *s2 && *s1 !='\0'){
    s1++;
    s2++;
  }
  t = (*(unsigned char*)s1 -*(unsigned char*)s2);
  return t;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  int t;
  for(size_t i = 1; i <= n; i++){
    if(*s1 == *s2 && *s1 !='\0'){
      s1++;
      s2++;
    }
    else{
      t = (*(unsigned char*)s1 -*(unsigned char*)s2);
      return t;
    }
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  for(size_t i = 1; i <= n; i++){
    *p = (unsigned char)c;
    p++;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char*)dst;
    const unsigned char *s = (const unsigned char*)src;
    if (d < s) {
        // 正向拷贝：从前往后
        for (size_t i = 1; i <= n; i++) {
            *d = *s;
            d++;
            s++;
        }
    } else if (d > s) {
        // 反向拷贝：从后往前
        d += n - 1;
        s += n - 1;
        for (size_t i = 1; i <= n; i++) {
            *d = *s;
            d--;
            s--;
        }
    }
    // 若 d = s，什么都不做
    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *o = (unsigned char *)out;
    const unsigned char *i = (const unsigned char *)in;
    for (size_t cnt = 1; cnt <= n; cnt++) {
        *o = *i;
        o++;
        i++;
    }
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (size_t i = 1; i <= n; i++) {
        if (*p1 != *p2) {
            return (int)(*p1 - *p2);
        }
        p1++;
        p2++;
    }
    return 0;
}

#endif
