#include <string.h>

char *stpncpy(char *restrict d, const char *restrict s, size_t n) {
  for (; n && (*d = *s); n--, s++, d++)
    ;
  memset(d, 0, n);
  return d;
}
