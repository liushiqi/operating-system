#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void *memchr(const void *src, int c, size_t n);
int memcmp(const void *lhs, const void *rhs, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
void *memset(void *dest, int ch, size_t count);

char *stpcpy(char *restrict d, const char *restrict s);
char *stpncpy(char *restrict d, const char *restrict s, size_t n);
char *strchr(const char *s, int c);
char *strchrnul(const char *s, int c);

int strcmp(const char *lhs, const char *rhs);
char *strcpy(char *dest, const char *src);
size_t strcspn(const char *s, const char *c);
size_t strlen(const char *src);
int strncmp(const char *_l, const char *_r, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *restrict dest, const char *restrict src);
size_t strnlen(const char *s, size_t n);
size_t strspn(const char *s, const char *c);
char *strcat(char *dest, const char *src);
char *strtok(char *restrict s, const char *restrict sep);

char *strerror(int errnum);

#endif /* STRING_H */
