#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_

#include <stdarg.h>
#include <stddef.h>

int printf(const char *format, ...) __attribute__((format(__printf__, 1, 2)));
int sprintf(char *str, const char *format, ...) __attribute__((format(__printf__, 2, 3)));
int snprintf(char *str, size_t size, const char *format, ...) __attribute__((format(__printf__, 3, 4)));

int vprintf(const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

int puts(const char *str);
int putchar(int ch);
int gets(char *line, size_t n);

#endif
