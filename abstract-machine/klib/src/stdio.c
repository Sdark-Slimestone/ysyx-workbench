#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *format, ...) {
  va_list ap;
    char i;
    va_start(ap, format);
    while ((i = *format++) != '\0') {
        if (i == '%') {
            i = *format++;
            /* 处理 'l' 和 'll' 长度修饰符 */
            if (i == 'l') {
                i = *format++;
                if (i == 'l') i = *format++; /* 跳过第二个 'l'（针对 'll'） */
            }
            if (i == '\0') break;
            switch (i) {
                case 'c': {
                    char ch = va_arg(ap, int);
                    putch(ch);
                    break;
                }
                case 's': {
                    char *sh = va_arg(ap, char *);
                    while (*sh) putch(*sh++);
                    break;
                }
                case 'd': {
                    int dh = va_arg(ap, int);
                    if (dh < 0) {
                        putch('-');
                        dh = -dh;
                    }
                    if (dh == 0) {
                        putch('0');
                        break;
                    }
                    int counter = 0;
                    int buf[12];
                    while (dh > 0) {
                        buf[counter] = dh % 10;
                        dh = dh / 10;
                        counter++;
                    }
                    for (counter = counter - 1; counter >= 0; counter--) {
                        putch(buf[counter] + '0');
                    }
                    break;
                }
                case 'o': {
                    unsigned int oh = va_arg(ap, unsigned int);
                    if (oh == 0) {
                        putch('0');
                        break;
                    }
                    int counter = 0;
                    int buf[12];
                    while (oh > 0) {
                        buf[counter] = oh % 8;
                        oh = oh / 8;
                        counter++;
                    }
                    for (counter = counter - 1; counter >= 0; counter--) {
                        putch(buf[counter] + '0');
                    }
                    break;
                }
                case 'x': {
                    unsigned int xh = va_arg(ap, unsigned int);
                    if (xh == 0) {
                        putch('0');
                        break;
                    }
                    int counter = 0;
                    int buf[8];
                    while (xh > 0) {
                        buf[counter] = xh % 16;
                        xh = xh / 16;
                        counter++;
                    }
                    for (counter = counter - 1; counter >= 0; counter--) {
                        if (buf[counter] > 9) {
                            putch(buf[counter] - 10 + 'a');
                        } else {
                            putch(buf[counter] + '0');
                        }
                    }
                    break;
                }
                case '%': {
                    putch('%');
                    break;
                }
            }
        } else {
            putch(i);
        }
    }
    va_end(ap);
    return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("未实现");
}

int sprintf(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *p = out;
    char c;
    while ((c = *fmt++) != '\0') {
        if (c != '%') {
            *p++ = c;
        } else {
            c = *fmt++;
            switch (c) {
                case 'c': {
                    char ch = (char)va_arg(ap, int);
                    *p++ = ch;
                    break;
                }
                case 's': {
                    char *s = va_arg(ap, char *);
                    while (*s) *p++ = *s++;
                    break;
                }
                case 'd': {
                    int d = va_arg(ap, int);
                    if (d < 0) {
                        *p++ = '-';
                        d = -d;
                    }
                    if (d == 0) {
                        *p++ = '0';
                        break;
                    }
                    char buf[12];
                    int idx = 0;
                    while (d > 0) {
                        buf[idx++] = '0' + (d % 10);
                        d /= 10;
                    }
                    while (idx > 0) *p++ = buf[--idx];
                    break;
                }
                case 'o': {
                    unsigned int o = va_arg(ap, unsigned int);
                    if (o == 0) {
                        *p++ = '0';
                        break;
                    }
                    char buf[12];
                    int idx = 0;
                    while (o > 0) {
                        buf[idx++] = '0' + (o % 8);
                        o /= 8;
                    }
                    while (idx > 0) *p++ = buf[--idx];
                    break;
                }
                case 'x': {
                    unsigned int x = va_arg(ap, unsigned int);
                    if (x == 0) {
                        *p++ = '0';
                        break;
                    }
                    char buf[12];
                    int idx = 0;
                    while (x > 0) {
                        int digit = x % 16;
                        buf[idx++] = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
                        x /= 16;
                    }
                    while (idx > 0) *p++ = buf[--idx];
                    break;
                }
                case '%': {
                    *p++ = '%';
                    break;
                }
                default:
                    /* 未知格式：原样输出 % 和字符 */
                    *p++ = '%';
                    *p++ = c;
                    break;
            }
        }
    }
    *p = '\0';
    va_end(ap);
    return p - out;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("未实现");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("未实现");
}

#endif